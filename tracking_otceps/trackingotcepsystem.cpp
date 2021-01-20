#include "trackingotcepsystem.h"

#include <qdebug.h>
#include <QtMath>
#include <assert.h>
#include "baseobjecttools.h"
#include "mvp_system.h"

#include "tos_speedcalc.h"
#include "m_otceps.h"
#include "m_zam.h"
#include "tos_rc.h"


TrackingOtcepSystem::TrackingOtcepSystem(QObject *parent,ModelGroupGorka *modelGorka,int trackingType) :
    BaseWorker(parent)
{
    this->modelGorka=modelGorka;
    this->trackingType=trackingType;
    iGetNewOtcep=nullptr;
    makeWorkers(modelGorka);
}


QList<BaseWorker *> TrackingOtcepSystem::makeWorkers(ModelGroupGorka *O)
{
    this->modelGorka=O;
    QList<BaseWorker *> l;
    // собираем все отцепы
    auto lotceps=modelGorka->findChildren<m_Otceps *>();
    if (!lotceps.isEmpty()) {
        auto otceps=lotceps.first();
        otceps->disableUpdateStates=true;
        foreach (auto *otcep, otceps->otceps()) {
            tos_OtcepData *od=new tos_OtcepData(this,otcep);
            lo.push_back(od);
            mNUM2OD[od->otcep->NUM()]=od;
            // отключаем собственную обработку
            otcep->disableUpdateStates=true;
            otcep->addTagObject(od,27);
        }
    }
    // собираем все РЦ
    auto l_rc=modelGorka->findChildren<m_RC*>();
    foreach (auto rc, l_rc) {
        tos_Rc*trc=new tos_Rc(this,rc);
        mRc2TRC[rc]=trc;
        l_tos_Rc.push_back(trc);
        m_RC_Gor_Park *rc_park=qobject_cast<m_RC_Gor_Park*>(rc);
        if (rc_park!=nullptr){
            tos_KzpTracking *kzpTracking=new tos_KzpTracking (this,trc);
            l_kzpt.push_back(kzpTracking);
            continue;
        }
        m_RC_Gor_ZKR *rc_zkr=qobject_cast<m_RC_Gor_ZKR*>(rc);
        if (rc_zkr!=nullptr){
            tos_ZkrTracking *zkrTracking=new tos_ZkrTracking (this,trc);
            l_zkrt.push_back(zkrTracking);
            continue;
        }
        tos_RcTracking *rct=new tos_RcTracking(this,trc);
        l_rct.push_back(rct);
    }
    // собираем замедлители для скорости входа выхода
    auto l_zam=modelGorka->findChildren<m_Zam*>();
    foreach (auto zam, l_zam) {
        mRc2Zam.insert(zam->rc(),zam);
    }
    // собираем РИС для текущей скорости
    foreach (auto rc, l_rc) {
        foreach (m_Base *m, rc->devices()) {
            m_RIS *ris=qobject_cast<m_RIS *>(m);
            if (ris!=nullptr)
                mRc2Ris.insert(rc,ris);
        }
    }

    resetStates();
    return l;
}

void TrackingOtcepSystem::resetStates()
{
    //otceps->resetStates();
    foreach (auto rct, l_rct) {
        rct->resetStates();
    }
    foreach (auto otcep, lo) {
        otcep->resetTracking();
    }
    foreach (auto *kzpt, l_kzpt) {
        kzpt->resetStates();
    }
    foreach (auto *zkrt, l_zkrt) {
        zkrt->resetStates();
    }


}
QList<SignalDescription> TrackingOtcepSystem::acceptOutputSignals()
{

    QList<SignalDescription> l;
    foreach (auto trc, l_tos_Rc) {
        l+=trc->acceptOutputSignals();
    }
    foreach (auto *zkrt, l_zkrt) {
        l+=zkrt->acceptOutputSignals();
    }
    return l;
}
void TrackingOtcepSystem::state2buffer()
{

    foreach (auto trc, l_tos_Rc) {
        trc->state2buffer();
    }
    foreach (auto *zkrt, l_zkrt) {
        zkrt->state2buffer();
    }
    // а мы ничего не посылаем для этого есть OtcepsController
    //    foreach (auto otcep, lo) {
    //        otcep->tos->state2buffer();
    //    }
}

tos_OtcepData *TrackingOtcepSystem::getNewOtcep(tos_Rc *trc)
{
    if (iGetNewOtcep!=nullptr){
        int num=iGetNewOtcep->getNewOtcep(trc->rc);
        if (num>0){
            return mNUM2OD[num];
        }
    }
    return nullptr;
}




void TrackingOtcepSystem::work(const QDateTime &T)
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

void TrackingOtcepSystem::checkOtcepComplete()
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

void TrackingOtcepSystem::checkOtcepSplit()
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

void TrackingOtcepSystem::updateOtcepsOnRc(const QDateTime &T)
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

void TrackingOtcepSystem::updateOtcepParams(tos_OtcepData *o,const QDateTime &T)
{

    auto otcep=o->otcep;
    if (!otcep->STATE_ENABLED()) return;
    if ((otcep->RCS==nullptr)&&(otcep->RCF==nullptr)&&(otcep->STATE_LOCATION()==m_Otcep::locationOnPrib)) return;
    if ((otcep->RCS==nullptr)&&(otcep->RCF==nullptr)){
        otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
        return;
    }
    int locat=m_Otcep::locationOnSpusk;

    // признак на зкр
    bool inzkr=false;
    foreach (auto zkrt, l_zkrt) {
        if (zkrt->rc_zkr==otcep->RCF) {
            inzkr=true;
            otcep->setSTATE_PUT_NADVIG(zkrt->rc_zkr->PUT_NADVIG());
        }
    }
    otcep->setSTATE_ZKR_PROGRESS(inzkr);

    // KZP
    m_RC_Gor_Park * rc_park=qobject_cast<m_RC_Gor_Park *>(otcep->RCF);
    if (rc_park!=nullptr){
        locat=m_Otcep::locationOnPark;
    }


    // скорость входа
    if (mRc2Zam.contains(otcep->RCS)){
        m_Zam *zam=mRc2Zam[otcep->RCS];
        int n=zam->NTP();
        if ((zam->TIPZM()==1) &&(otcep->STATE_V_INOUT(0,n)==_undefV_)) otcep->setSTATE_V_INOUT(0,n,zam->ris()->STATE_V());
    }
    // скорость выхода
    if (mRc2Zam.contains(otcep->RCF)){
        m_Zam *zam=mRc2Zam[otcep->RCF];
        int n=zam->NTP();
        if ((otcep->STATE_V_INOUT(1,n)==_undefV_)) otcep->setSTATE_V_INOUT(1,n,zam->ris()->STATE_V());

    }
    qreal Vars=_undefV_;
    foreach (auto rc, otcep->vBusyRc) {
        if (!mRc2Zam.contains(rc)) continue;
        m_Zam *zam=mRc2Zam[rc];
        int n=zam->NTP();
        // режим торм
        if (zam->STATE_STUPEN()>0){
            if (zam->STATE_STUPEN()>otcep->STATE_OT_RA(0,n)) otcep->setSTATE_OT_RA(0,n,zam->STATE_STUPEN());
        }
        if (zam->STATE_A()!=0){
            otcep->setSTATE_OT_RA(1,n,zam->STATE_A());
        }
        if (mRc2Ris.contains(rc)){
            m_RIS *ris=mRc2Ris[rc];
            if (ris->controllerARS()->isValidState()){
                Vars=ris->STATE_V();
                if (Vars<1.3) Vars=0;
            }
        }

    }
    if ((qFabs(otcep->STATE_V_ARS()-Vars)>=0.4)) {
        otcep->setSTATE_V_ARS(Vars);
    }
    // маршрут
    m_RC_Gor*rc=qobject_cast<m_RC_Gor*>(otcep->RCS);
    if ((rc==nullptr)||
            ((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)&&
             (otcep->STATE_MAR()!=0)&&
             ((otcep->STATE_MAR()<rc->MINWAY())||(otcep->STATE_MAR()>rc->MAXWAY()))
             )
            )otcep->setSTATE_ERROR(true); else otcep->setSTATE_ERROR(false);
    int marf=0;
    while (rc!=nullptr){
        if (rc->MINWAY()==rc->MAXWAY()){
            marf=rc->MINWAY();
            break;
        }
        rc=qobject_cast<m_RC_Gor*>(rc->next_rc[_forw]);
    }
    otcep->setSTATE_MAR_F(marf);



    otcep->setSTATE_LOCATION(locat);

    // финализируем скорость отцепов
    if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk){
            o->updateV_RC(T);
    }
    otcep->setSTATE_V(o->STATE_V());


//    o->calcLenByRc();


}

void TrackingOtcepSystem::resetTracking(int num)
{
    if (mNUM2OD.contains(num)){
        auto otcep=mNUM2OD[num];
        otcep->resetTracking();
    }
    foreach (auto trc, l_tos_Rc) {
        if (trc->l_otceps.contains(num)) trc->l_otceps.removeAll(num);
        foreach (auto od, trc->l_od) {
            if (od.num==num) trc->l_od.removeOne(od);
        }
    }
}

void TrackingOtcepSystem::resetTracking()
{
    foreach (auto otcep,lo){
        otcep->resetTracking();
    }
    foreach (auto trc, l_tos_Rc) {
        trc->l_otceps.clear();
        trc->l_od.clear();
    }
}





