#include "tos_system_dso.h"

#include <qdebug.h>
#include <QtMath>
#include <assert.h>
#include "baseobjecttools.h"
#include "mvp_system.h"

#include "tos_speedcalc.h"
#include "m_otceps.h"
#include "m_zam.h"


tos_System_DSO::tos_System_DSO(QObject *parent) :
    tos_System(parent)
{
    iGetNewOtcep=nullptr;
    makeWorkers(modelGorka);
}


void tos_System_DSO::makeWorkers(ModelGroupGorka *modelGorka)
{
    tos_System::makeWorkers(modelGorka);
    // собираем все ДСО

    l_dso=modelGorka->findChildren<m_DSO_RD_21*>();

    foreach (auto dso, l_dso) {
        tos_DSO *tdso=new tos_DSO(this,dso);
        l_tdso.push_back(tdso);
        mDSO2TDSO[dso]=tdso;
        if ((dso->rc_next[0]!=nullptr)&&((dso->rc_next[1]!=nullptr))){
            // проверяем на повтор
            bool ex=false;
            foreach (auto trdso1, l_trdso) {
                if ((trdso1->tdso->dso->rc_next[0]==dso->rc_next[0])&&
                        (trdso1->tdso->dso->rc_next[1]==dso->rc_next[1])){
                    trdso1->add2DSO(tdso);
                    mDSO2TRDSO[dso]=trdso1;
                    ex=true;
                    break;
                }
            }
            if (!ex) {
                tos_DsoTracking *trdso=new tos_DsoTracking(this,tdso);
                mDSO2TRDSO[dso]=trdso;
                l_trdso.push_back(trdso);
            }

        }
    }
    foreach (auto zkr, l_zkr) {
        tos_Zkr_DSO *tzkr=new tos_Zkr_DSO(this,mRc2TRC[zkr]);
        l_tzkr.push_back(tzkr);
    }



    resetStates();
}

void tos_System_DSO::resetStates()
{
    tos_System::resetStates();
    foreach (auto w, l_dso) {
        w->resetStates();
    }
    foreach (auto w, l_tdso) {
        w->resetStates();
    }
    foreach (auto w, l_trdso) {
        w->resetStates();
    }
    foreach (auto w, l_trdso) {
        w->resetStates();
    }
    foreach (auto w, l_tzkr) {
        w->resetStates();
    }



}
QList<SignalDescription> tos_System_DSO::acceptOutputSignals()
{

    auto l=tos_System::acceptOutputSignals();

    foreach (auto trc, l_tos_Rc) {
        trc->rc->setSIGNAL_BUSY_DSO(trc->rc->SIGNAL_BUSY_DSO().innerUse());
        trc->rc->setSIGNAL_BUSY_DSO_ERR(trc->rc->SIGNAL_BUSY_DSO_ERR().innerUse());
        trc->rc->setSIGNAL_INFO_DSO(trc->rc->SIGNAL_INFO_DSO().innerUse());
        l<<trc->rc->SIGNAL_BUSY_DSO()<<trc->rc->SIGNAL_BUSY_DSO_ERR()<<trc->rc->SIGNAL_INFO_DSO();
    }

    foreach (auto w, l_tdso) {
        l+=w->acceptOutputSignals();
    }
    foreach (auto w, l_trdso) {
        l+=w->acceptOutputSignals();
    }
    foreach (auto w, l_tzkr) {
        l+=w->acceptOutputSignals();
    }
    return l;
}
void tos_System_DSO::state2buffer()
{
    tos_System::state2buffer();

    foreach (auto trc, l_tos_Rc) {
        trc->rc->SIGNAL_BUSY_DSO().setValue_1bit(trc->rc->STATE_BUSY_DSO());
        trc->rc->SIGNAL_BUSY_DSO_ERR().setValue_1bit(trc->rc->STATE_BUSY_DSO_ERR());
        DSO_Data d;
        d.V=trc->l_os.size();
        trc->rc->SIGNAL_INFO_DSO().setValue_data(&d,sizeof (d));

    }

    foreach (auto w, l_tdso) {
        w->state2buffer();
    }
    foreach (auto w, l_trdso) {
        w->state2buffer();
    }
    foreach (auto w, l_tzkr) {
        w->state2buffer();
    }
}


void tos_System_DSO::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;
    // состояние рц
    foreach (auto trc, l_tos_Rc) {
        trc->work(T);
    }
    // датчики
    foreach (auto w, l_tdso) {
        w->work(T);
    }
    // переходы
    foreach (auto w, l_trdso) {
        w->work(T);
    }

    // зкр
    foreach (auto w, l_tzkr) {
        w->work(T);
    }


    // кзп

    setDSOBUSY();


    // раставляем по рц
    //updateOtcepsOnRc(T);

    // выставляем параметра отцепа
    foreach (auto otcep, lo) {
        updateOtcepParams(otcep,T);
    }

}


void tos_System_DSO::updateOtcepsOnRc(const QDateTime &T)
{
    // проставялем начало и конец по номеру оси

    foreach (auto o, lo) {
        m_RC *rcs=nullptr;
        m_RC *rcf=nullptr;
        TOtcepDataOs s_os;
        TOtcepDataOs f_os;

        foreach (auto trc, l_tos_Rc) {
            if (o->otcep->STATE_LOCATION()==m_Otcep::locationUnknow) continue;
            if (o->otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) continue;
            for (TOtcepDataOs &os :trc->l_os){
                if (os.num==o->otcep->NUM()){
                    if ((!rcs)||(os.os_otcep<s_os.os_otcep)){
                        rcs=trc->rc;s_os=os;
                    }
                    if ((!rcf)||(os.os_otcep>f_os.os_otcep)){
                        rcf=trc->rc;f_os=os;
                    }
                }
            }
        }
        if ((rcs==nullptr)&&(o->otcep->RCS!=nullptr)){
            resetTracking(o->otcep->NUM());
        } else {
            if ((rcs!=o->otcep->RCS) || (rcf!=o->otcep->RCF)){
                o->otcep->RCS=rcs;
                o->otcep->RCF=rcf;
                o->otcep->setBusyRC();
                if (s_os.t>f_os.t) o->otcep->setSTATE_DIRECTION(s_os.d); else
                    o->otcep->setSTATE_DIRECTION(f_os.d);
            }
        }
    }

    foreach (auto trc, l_tos_Rc) {
        trc->l_otceps.clear();
        for (TOtcepDataOs &os :trc->l_os){
            if (!trc->l_otceps.contains(os.num)) {
                trc->l_otceps.push_back(os.num);
            }
        }
    }
}

void tos_System_DSO::resetTracking(int num)
{
    tos_System::resetTracking(num);

    foreach (auto trc, l_tos_Rc) {
        if (trc->l_otceps.contains(num)) trc->l_otceps.removeAll(num);
        for (TOtcepDataOs &os :trc->l_os){
            if (os.num==num) os.num=0;
        }
    }
}

void tos_System_DSO::resetTracking()
{
    tos_System::resetTracking();
    //
    foreach (auto trc, l_tos_Rc) {
        trc->l_otceps.clear();
        for (TOtcepDataOs &os :trc->l_os){
            os.num=0;
        }
    }

    foreach (auto w, l_dso) {
        w->resetStates();
    }
    foreach (auto w, l_tdso) {
        w->resetStates();
    }
    foreach (auto w, l_trdso) {
        w->resetStates();
    }
    foreach (auto w, l_trdso) {
        w->resetStates();
    }
    foreach (auto w, l_tzkr) {
        w->resetStates();
    }
}

bool tos_System_DSO::resetDSOBUSY(QString idtsr, QString &acceptStr)
{
    int n=0;
    foreach (auto trc, l_tos_Rc) {
        if ((idtsr=="*") || (trc->rc->idstr()==idtsr)){
            trc->l_os.clear();
            n++;
        }
    }
    acceptStr=QString("Снята занятость на %1 РЦ .").arg(n);
    updateOtcepsOnRc(QDateTime());
    return true;
}

void tos_System_DSO::setDSOBUSY()
{
    foreach (auto trc, l_tos_Rc) {
        if (trc->l_os.isEmpty())
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::free); else
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::busy);
    }
}

TOtcepDataOs tos_System_DSO::moveOs(tos_Rc *rc0, tos_Rc *rc1, int d,const QDateTime &T)
{
    //  0 <-- 1  <<D0
    TOtcepDataOs moved_os;
    if (d==_forw){
        if ((rc1==nullptr) || (rc1->l_os.isEmpty())){
            // рождение ноаовой оси
            TOtcepDataOs new_os;

            moved_os=new_os;
        } else {
            moved_os=rc1->l_os.first();
            rc1->l_os.pop_front();

        }
        moved_os.t=T;
        moved_os.d=d;
        if (rc0!=nullptr) {
            rc0->l_os.push_back(moved_os);
            if (moved_os.num!=0){
                if (!rc0->l_otceps.contains(moved_os.num))
                    rc0->l_otceps.push_back(moved_os.num);
            }
        }
    }
    //  0 --> 1  <<D0
    if (d==_back){
        if ((rc0==nullptr) || (rc0->l_os.isEmpty())){
            // рождение ноаовой оси
            TOtcepDataOs new_os;
            moved_os=new_os;
        } else {
            moved_os=rc0->l_os.last();
            rc0->l_os.pop_back();

        }
        moved_os.t=T;
        moved_os.d=d;
        if (rc1!=nullptr) {
            rc1->l_os.push_front(moved_os);
            if (moved_os.num!=0){
                if (!rc0->l_otceps.contains(moved_os.num))
                    rc0->l_otceps.push_front(moved_os.num);
            }
        }
    }
    return moved_os;
}


tos_Rc *tos_System_DSO::getRc(TOtcepDataOs os)
{
    foreach (auto trc, l_tos_Rc) {
        foreach (auto os1, trc->l_os) {
            if (os1==os) return  trc;
        }
    }return nullptr;
}
