#include "dynamicotcepsystem.h"
#include "tos_speedcalc.h"
#include "m_rc_gor_zkr.h"
#include "m_rc_gor_park.h"
#include "m_ris.h"
#include <qfileinfo.h>
#include <QtMath>
#include "dynamicstatistic.h"

DynamicOtcepSystem::DynamicOtcepSystem(QObject *parent, TrackingOtcepSystem *TOS) : BaseWorker(parent)
{
    this->TOS=TOS;
    connect(TOS,&TrackingOtcepSystem::otcep_rcsf_change,this,&DynamicOtcepSystem::slot_otcep_rcsf_change);

    l_rc_park=TOS->modelGorka->findChildren<m_RC_Gor_Park*>();
}

void DynamicOtcepSystem::calculateXoffset(m_Otcep *otcep, const QDateTime &T,int sf)
{
    if (sf==0){
        if (otcep->RCS!=nullptr){
            if (otcep->tos->dos_dt_RCS_XOFFSET.isValid()){
                qreal xoffset=otcep->STATE_D_RCS_XOFFSET();
                if (otcep->STATE_V()!=_undefV_){
                    qint64 ms=otcep->tos->dos_dt_RCS_XOFFSET.msecsTo(T);
                    if (ms>0){
                        xoffset=xoffset+tos_SpeedCalc::vt_len_m(otcep->STATE_V(),ms);
                    }
                    otcep->tos->dos_dt_RCS_XOFFSET=T;
                    if (xoffset>otcep->RCS->LEN()) xoffset=otcep->RCS->LEN();
                    if (xoffset<0) xoffset=otcep->RCS->LEN();;
                } else {
                    xoffset=otcep->RCS->LEN();
                }
                otcep->setSTATE_D_RCS_XOFFSET(xoffset);
            }
        }
    }
    if (sf==1){
        if (otcep->RCF!=nullptr){
            if (otcep->tos->dos_dt_RCF_XOFFSET.isValid()){
                qreal xoffset=otcep->STATE_D_RCF_XOFFSET();
                if (otcep->STATE_V()!=_undefV_){
                    qint64 ms=otcep->tos->dos_dt_RCF_XOFFSET.msecsTo(T);
                    if (ms>0){
                        xoffset=xoffset+tos_SpeedCalc::vt_len_m(otcep->STATE_V(),ms);
                    }
                    otcep->tos->dos_dt_RCF_XOFFSET=T;
                    if (xoffset>otcep->RCF->LEN()) xoffset=0;
                    if (xoffset<0) xoffset=0;
                } else {
                    xoffset=0;
                }
                otcep->setSTATE_D_RCF_XOFFSET(xoffset);
            }
        }
    }
}

void DynamicOtcepSystem::calculateXoffsetKzp(const QDateTime &T)
{
    foreach (m_RC_Gor_Park*rc_park, l_rc_park) {

        // пересчитываем смещения
        bool recalc=false;
        if (!rc_park->rcs->l_otceps.isEmpty()){
            qreal end_x=rc_park->STATE_KZP_D();
            m_Otcep * o1=rc_park->rcs->l_otceps.last();
            if (o1->RCF==rc_park){
                if (end_x!=o1->STATE_D_RCF_XOFFSET()){
                    recalc=true;
                    o1->setSTATE_D_RCF_XOFFSET(end_x);
                    o1->setSTATE_D_RCS_XOFFSET(end_x+o1->STATE_LEN());
                    end_x=o1->STATE_D_RCS_XOFFSET();
                }
                o1->setSTATE_V_KZP(rc_park->STATE_KZP_V());
            }
            for (int i=rc_park->rcs->l_otceps.size()-2;i>=0;i--){
                m_Otcep * o=rc_park->rcs->l_otceps[i];
                // prognoz_KZP(o,T);// тут должен быть прогноз положения хвоста
                if (recalc){
                    if (dynamicStatistic!=nullptr){
                        qint64 ms=o->tos->out_tp_t.msecsTo(T);
                        qreal x=0;
                        qreal v;
                        dynamicStatistic->prognoz_ms_xv(o,o->RCF,ms,x,v);
                        if (x>o->STATE_D_RCF_XOFFSET()){
                            o->setSTATE_D_RCF_XOFFSET(x);
                            o->setSTATE_D_RCS_XOFFSET(x+o->STATE_LEN());
                        }
                    }
                }
                if (o->STATE_D_RCF_XOFFSET()<end_x) {
                    o->setSTATE_D_RCF_XOFFSET(end_x);
                    o->setSTATE_D_RCS_XOFFSET(end_x+o->STATE_LEN());
                    end_x=o->STATE_D_RCS_XOFFSET();
                }
            }
        }
    }
}







void DynamicOtcepSystem::slot_otcep_rcsf_change(m_Otcep *otcep,int sf, m_RC *, m_RC *rcTo, QDateTime T, QDateTime )
{
    if (sf==0){
        if ((otcep->STATE_V_RC()!=_undefV_)&&(rcTo)){
            otcep->setSTATE_D_RCS_XOFFSET(0.001);
            otcep->tos->dos_dt_RCS_XOFFSET=T;
        } else {
            otcep->setSTATE_D_RCS_XOFFSET(-1);
            otcep->tos->dos_dt_RCS_XOFFSET=QDateTime();
        }
    }
    if (sf==1){
        if ((otcep->STATE_V_RC()!=_undefV_)&&(rcTo)){
            otcep->setSTATE_D_RCF_XOFFSET(0.001);
            otcep->tos->dos_dt_RCF_XOFFSET=T;
        } else {
            otcep->setSTATE_D_RCF_XOFFSET(-1);
            otcep->tos->dos_dt_RCF_XOFFSET=QDateTime();
        }

    }

}



void DynamicOtcepSystem::work(const QDateTime &T)
{

        foreach (m_Otcep *otcep, TOS->lo) {
        if ((otcep->RCF)&&(qobject_cast<m_RC_Gor_Park*>(otcep->RCF)!=nullptr)){
            // кзп

            continue;
        }
        if ((otcep->RCS)&&(qobject_cast<m_RC_Gor_ZKR*>(otcep->RCS)!=nullptr)){
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
        otcep->setSTATE_D_RCS_XOFFSET(-1);
        otcep->setSTATE_D_RCF_XOFFSET(-1);
        otcep->tos->dos_dt_RCS_XOFFSET=QDateTime();
        otcep->tos->dos_dt_RCF_XOFFSET=QDateTime();
    }
}
