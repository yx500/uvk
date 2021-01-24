#include "tos_system_rc.h"

#include <qdebug.h>
#include <QtMath>
#include <assert.h>
#include "baseobjecttools.h"
#include "mvp_system.h"

#include "tos_speedcalc.h"
#include "m_otceps.h"
#include "m_zam.h"


tos_System_RC::tos_System_RC(QObject *parent) :
    tos_System(parent)
{
    iGetNewOtcep=nullptr;
}


void tos_System_RC::makeWorkers(ModelGroupGorka *modelGorka)
{
    tos_System::makeWorkers(modelGorka);
    // собираем все РЦ
    foreach (auto trc, l_tos_Rc) {

        m_RC_Gor_Park *rc_park=qobject_cast<m_RC_Gor_Park*>(trc->rc);
        if (rc_park!=nullptr){
            tos_KzpTracking *kzpTracking=new tos_KzpTracking (this,trc);
            l_kzpt.push_back(kzpTracking);
            continue;
        }
        m_RC_Gor_ZKR *rc_zkr=qobject_cast<m_RC_Gor_ZKR*>(trc->rc);
        if (rc_zkr!=nullptr){
            tos_ZkrTracking *zkrTracking=new tos_ZkrTracking (this,trc);
            l_zkrt.push_back(zkrTracking);
            continue;
        }
        tos_RcTracking *rct=new tos_RcTracking(this,trc);
        l_rct.push_back(rct);
    }
    resetStates();
}

void tos_System_RC::resetStates()
{
    tos_System::resetStates();
    foreach (auto rct, l_rct) {
        rct->resetStates();
    }
    foreach (auto *kzpt, l_kzpt) {
        kzpt->resetStates();
    }
    foreach (auto *zkrt, l_zkrt) {
        zkrt->resetStates();
    }


}
QList<SignalDescription> tos_System_RC::acceptOutputSignals()
{

    auto l=tos_System::acceptOutputSignals();
    foreach (auto trc, l_tos_Rc) {
        l+=trc->acceptOutputSignals();
    }
    foreach (auto *zkrt, l_zkrt) {
        l+=zkrt->acceptOutputSignals();
    }
    return l;
}
void tos_System_RC::state2buffer()
{
    tos_System::state2buffer();
    foreach (auto trc, l_tos_Rc) {
        trc->state2buffer();
    }
    foreach (auto *zkrt, l_zkrt) {
        zkrt->state2buffer();
    }
}





void tos_System_RC::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;
    // состояние рц
    foreach (auto trc, l_tos_Rc) {
        trc->work(T);
    }

    // зкр
    foreach (tos_ZkrTracking *zkrt, l_zkrt) {
        zkrt->work(T);
    }

    // кзп
    foreach (tos_KzpTracking *kzpt, l_kzpt) {
        kzpt->work(T);
    }


    // механизм таблицы переходов
    foreach (auto rct, l_rct) {
        rct->prepareCurState();
    }
    // следим по РЦ
    // двигаем нормально вперед
    foreach (auto rct, l_rct) {
        rct->workOtcep(_forw,T,0);
    }
    // двигаем нормально назад
    foreach (auto rct, l_rct) {
        rct->workOtcep(_back,T,1);
    }
    // двигаем  вперед  исключения
    foreach (auto rct, l_rct) {
        rct->workOtcep(_forw,T,2);
    }
    // двигаем  назад  исключения
    foreach (auto rct, l_rct) {
        rct->workOtcep(_back,T,3);
    }

    // раставляем по рц
    updateOtcepsOnRc(T);

    // выставляем параметра отцепа
    foreach (auto otcep, lo) {
        updateOtcepParams(otcep,T);
    }

}

void tos_System_RC::checkOtcepComplete()
{
    foreach (auto trc, l_tos_Rc) {
        if (!trc->l_od.isEmpty()){
            auto od=trc->l_od.last();
            if (od.p==_pOtcepStart){
                bool ex=false;
                auto trc_prev=trc->next_rc[_back];
                int grafSize=0;
                while(trc_prev!=nullptr){
                    if (!trc_prev->l_od.isEmpty()){
                        if (trc_prev->l_od.first().num==od.num)  {ex=true;break;}
                        break;
                    }
                    trc_prev=trc->next_rc[_back];
                    if (++grafSize>100) break;
                }
                if (!ex){
                    // удаляем хвост
                    foreach (auto trc1, l_tos_Rc) {
                        foreach (auto od1, trc1->l_od) {
                            if ((od.num==od1.num)&&(od1.p==_pOtcepEnd)){
                                trc1->remove_od(od1);
                            }
                        }
                    }
                    // создаем новый
                    auto odn=od;
                    odn.p=_pOtcepEnd;
                    odn.track=_broken_track;
                    // до последней занятости
                    auto trc_end=trc;
                    trc_prev=trc->next_rc[_back];
                    while(trc_prev!=nullptr){
                        if ((trc_prev->STATE_BUSY!=MVP_Enums::free)||(!trc_prev->l_od.isEmpty())){
                            trc_end->l_od.push_back(odn);
                            break;
                        }
                        trc_end=trc_prev;
                        trc_prev=trc->next_rc[_back];
                    }
                    if (mNUM2OD.contains(odn.num))
                        mNUM2OD[odn.num]->otcep->setSTATE_ERROR_TRACK(true);
                }
            }
        }
    }
}

void tos_System_RC::checkOtcepSplit()
{
    // проверяем чтоб не было 2ух свободных рц по середине
    foreach (auto trc, l_tos_Rc) {
        if (!trc->l_od.isEmpty()){
            auto od=trc->l_od.last();
            if (od.p==_pOtcepStart){
                bool ex=false;
                auto trc_prev=trc->next_rc[_back];
                int grafSize=0;
                tos_Rc *trc3[3];
                trc3[0]=trc;trc3[1]=nullptr;trc3[2]=nullptr;

                while(trc_prev!=nullptr){
                    if (!trc_prev->l_od.isEmpty()){
                        break;
                    }
                    if ((trc3[1]!=nullptr)&&(trc3[2]!=nullptr)&&
                            (trc3[1]->STATE_BUSY==MVP_Enums::free)&&(trc3[2]->STATE_BUSY==MVP_Enums::free)&&
                            (!trc3[1]->STATE_ERR_LS) && (!trc3[2]->STATE_ERR_LS)){
                        ex=true;break;
                    }
                    trc_prev=trc->next_rc[_back];
                    if (++grafSize>100) break;
                    trc3[2]=trc3[1];trc3[1]=trc3[0];trc3[0]=trc_prev;

                }
                if (ex){
                    // удаляем хвост
                    foreach (auto trc1, l_tos_Rc) {
                        foreach (auto od1, trc1->l_od) {
                            if ((od.num==od1.num)&&(od1.p==_pOtcepEnd)){
                                trc1->remove_od(od1);
                            }
                        }
                    }
                    // создаем новый
                    auto odn=od;
                    odn.p=_pOtcepEnd;
                    odn.track=_broken_track;
                    // до последней занятости
                    auto trc_end=trc3[0];
                    trc_end->l_od.push_back(odn);
                    if (mNUM2OD.contains(odn.num))
                        mNUM2OD[odn.num]->otcep->setSTATE_ERROR_TRACK(true);
                }
            }
        }
    }

}

void tos_System_RC::updateOtcepsOnRc(const QDateTime &T)
{
    checkOtcepComplete();
    checkOtcepSplit();

    foreach (auto trc, l_tos_Rc) {
        trc->l_otceps.clear();
        foreach (auto od, trc->l_od) {
            if (!trc->l_otceps.contains(od.num)) {
                trc->l_otceps.push_back(od.num);
            }
        }
    }
    foreach (auto trc, l_tos_Rc) {
        if (!trc->l_od.isEmpty()){
            auto od=trc->l_od.last();
            if (od.p==_pOtcepStart){
                bool ex=false;
                auto trc_prev=trc->next_rc[_back];
                while(trc_prev!=nullptr){
                    if (!trc_prev->l_od.isEmpty()){
                        if (trc_prev->l_od.first().num==od.num)  {ex=true;break;}
                        break;
                    }
                    trc_prev=trc->next_rc[_back];
                }
                if (!ex){
                    qDebug() <<"updateOtcepsOnRc() not found end";
                } else {
                    auto trc_prev=trc->next_rc[_back];
                    while(trc_prev!=nullptr){
                        if (!trc_prev->l_od.isEmpty()) break;
                        trc_prev->l_otceps.push_back(od.num);
                        trc_prev=trc->next_rc[_back];
                    }
                }

            }
        }
    }


    foreach (auto trc, l_tos_Rc) {
        foreach (auto od, trc->l_od) {
            if (!trc->l_otceps.contains(od.num)) {
                if (mNUM2OD.contains(od.num)){
                    if ((od.p==_pOtcepStart)||(od.p==_pOtcepEnd)){
                        auto otcep=mNUM2OD[od.num];
                        if (trc->rc!=otcep->otcep->RCS){
                            otcep->setOtcepSF(od.p,trc->rc,od.d,T,od.track);
                        }
                    }
                }
            }
        }
    }
}

void tos_System_RC::resetTracking(int num)
{
    tos_System::resetTracking(num);

    foreach (auto trc, l_tos_Rc) {
        if (trc->l_otceps.contains(num)) trc->l_otceps.removeAll(num);
        foreach (auto od, trc->l_od) {
            if (od.num==num) trc->l_od.removeOne(od);
        }
    }
}

void tos_System_RC::resetTracking()
{
    tos_System::resetTracking();

    foreach (auto trc, l_tos_Rc) {
        trc->l_otceps.clear();
        trc->l_od.clear();
    }
}





