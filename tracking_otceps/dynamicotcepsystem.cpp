#include "dynamicotcepsystem.h"
#include "tos_speedcalc.h"
#include "m_rc_gor_zkr.h"
#include "m_rc_gor_park.h"
#include "m_ris.h"
DynamicOtcepSystem::DynamicOtcepSystem(QObject *parent, TrackingOtcepSystem *TOS) : BaseWorker(parent)
{
    this->TOS=TOS;
    // собираем РИС для скорости
    auto l_rc=TOS->modelGorka->findChildren<m_RC*>();
    foreach (auto rc, l_rc) {
        foreach (m_Base *m, rc->devices()) {
            m_RIS *ris=qobject_cast<m_RIS *>(m);
            if (ris!=nullptr)
                mRc2Ris.insert(rc,ris);
        }
    }
    l_rc_park=TOS->modelGorka->findChildren<m_RC_Gor_Park*>();
}

void DynamicOtcepSystem::calculateXoffset(m_Otcep *otcep, const QDateTime &T,int sf)
{
    if (sf==0){
        if (otcep->RCS!=nullptr){
            if (otcep->tos->dos_dt_RCS_XOFFSET.isValid()){
                qreal xoffset=otcep->STATE_RCS_XOFFSET();
                if (otcep->STATE_V()!=_undefV_){
                    qint64 ms=otcep->tos->dos_dt_RCS_XOFFSET.msecsTo(T);
                    if (ms>0){
                        xoffset=xoffset+tos_SpeedCalc::vt_len(otcep->STATE_V(),ms);
                    }
                    otcep->tos->dos_dt_RCS_XOFFSET=T;
                    if (xoffset>otcep->RCS->LEN()) xoffset=otcep->RCS->LEN();
                    if (xoffset<0) xoffset=otcep->RCS->LEN();;
                } else {
                    xoffset=otcep->RCS->LEN();
                }
                otcep->setSTATE_RCS_XOFFSET(xoffset);
            }
        }
    }
    if (sf==1){
        if (otcep->RCF!=nullptr){
            if (otcep->tos->dos_dt_RCF_XOFFSET.isValid()){
                qreal xoffset=otcep->STATE_RCF_XOFFSET();
                if (otcep->STATE_V()!=_undefV_){
                    qint64 ms=otcep->tos->dos_dt_RCF_XOFFSET.msecsTo(T);
                    if (ms>0){
                        xoffset=xoffset+tos_SpeedCalc::vt_len(otcep->STATE_V(),ms);
                    }
                    otcep->tos->dos_dt_RCF_XOFFSET=T;
                    if (xoffset>otcep->RCF->LEN()) xoffset=0;
                    if (xoffset<0) xoffset=0;
                } else {
                    xoffset=0;
                }
                otcep->setSTATE_RCF_XOFFSET(xoffset);
            }
        }
    }
}

void DynamicOtcepSystem::calculateXoffsetKzp(const QDateTime &T)
{
    foreach (m_RC_Gor_Park*rc_park, l_rc_park) {

        // пересчитываем смещения
        if (!rc_park->rcs->l_otceps.isEmpty()){
            qreal end_x=rc_park->STATE_KZP_D();
            m_Otcep * o1=rc_park->rcs->l_otceps.last();
            if (o1->RCF==rc_park){
                if (end_x!=o1->STATE_RCF_XOFFSET()){
                    o1->setSTATE_RCF_XOFFSET(end_x);
                    o1->setSTATE_RCS_XOFFSET(end_x+o1->STATE_LEN());
                    end_x=o1->STATE_RCS_XOFFSET();
                }
                o1->setSTATE_V_KZP(rc_park->STATE_KZP_V());
            }
            for (int i=rc_park->rcs->l_otceps.size()-2;i>=0;i--){
                m_Otcep * o=rc_park->rcs->l_otceps[i];
                prognoz_KZP(o,T);// тут должен быть прогноз положения хвоста

                if (o->STATE_RCF_XOFFSET()<end_x) {
                    o->setSTATE_RCF_XOFFSET(end_x);
                    o->setSTATE_RCS_XOFFSET(end_x+o->STATE_LEN());
                    end_x=o->STATE_RCS_XOFFSET();
                }
            }
        }
    }
}

void DynamicOtcepSystem::prognoz_KZP(m_Otcep *, const QDateTime &)
{
    // тут должен быть прогноз положения хвоста
}
void DynamicOtcepSystem::updateV_ARS(m_Otcep *otcep, const QDateTime &T)
{
    Q_UNUSED(T)
    qreal Vars=_undefV_;
    foreach (m_RC *rc, otcep->vBusyRc) {
        if (mRc2Ris.contains(rc)){
            m_RIS *ris=mRc2Ris[rc];
            if (ris->controllerARS()->isValidState()){
                Vars=ris->STATE_V();
                if (Vars<1.3) Vars=0;
                break;
            }
        }
    }
    if ((fabs(otcep->STATE_V_ARS()-Vars)>=0.4)) {
        otcep->setSTATE_V_ARS(Vars);
    }


}


void DynamicOtcepSystem::work(const QDateTime &T)
{
    foreach (m_Otcep *otcep, TOS->lo) {
        if (otcep->RCS!=otcep->tos->dos_RCS){
            otcep->tos->dos_RCS=otcep->RCS;
            if ((otcep->STATE_V_RC()!=_undefV_)&&(otcep->RCS)){
                otcep->setSTATE_RCS_XOFFSET(0);
                otcep->tos->dos_dt_RCS_XOFFSET=T;
            } else {
                otcep->setSTATE_RCS_XOFFSET(-1);
                otcep->tos->dos_dt_RCS_XOFFSET=QDateTime();
            }
        }
        if (otcep->RCF!=otcep->tos->dos_RCF){
            otcep->tos->dos_RCF=otcep->RCF;
            if ((otcep->STATE_V_RC()!=_undefV_)&&(otcep->RCF)){
                otcep->setSTATE_RCF_XOFFSET(0);
                otcep->tos->dos_dt_RCF_XOFFSET=T;
            } else {
                otcep->setSTATE_RCF_XOFFSET(-1);
                otcep->tos->dos_dt_RCF_XOFFSET=QDateTime();
            }
        }
    }
    qreal V=_undefV_;
    foreach (m_Otcep *otcep, TOS->lo) {
        updateV_ARS(otcep,T);
        if (otcep->STATE_V_ARS()!=_undefV_) V=otcep->STATE_V_ARS(); else
            V=otcep->STATE_V_RC();
        otcep->setSTATE_V(V);
    }
    foreach (m_Otcep *otcep, TOS->lo) {
        if ((otcep->RCF)&&(qobject_cast<m_RC_Gor_Park*>(otcep->RCF)!=nullptr)){
            // кзп
            continue;
        }
        if ((otcep->RCS)&&(qobject_cast<m_RC_Gor_ZKR*>(otcep->RCF)!=nullptr)){
            // zkr
            continue;
        }
        if ((otcep->RCF)&&(qobject_cast<m_RC_Gor_ZKR*>(otcep->RCF)!=nullptr)){
            // zkr
            calculateXoffset(otcep,T,0);
            continue;
        }
        calculateXoffset(otcep,T,0);
        calculateXoffset(otcep,T,1);

    }
    calculateXoffsetKzp(T);

}

void DynamicOtcepSystem::resetStates()
{
    BaseWorker::resetStates();
    foreach (m_Otcep *otcep, TOS->lo) {
        otcep->tos->dos_RCS=nullptr;
        otcep->tos->dos_RCF=nullptr;
        otcep->setSTATE_RCS_XOFFSET(-1);
        otcep->setSTATE_RCF_XOFFSET(-1);
        otcep->tos->dos_dt_RCS_XOFFSET=QDateTime();
        otcep->tos->dos_dt_RCF_XOFFSET=QDateTime();
    }
}
