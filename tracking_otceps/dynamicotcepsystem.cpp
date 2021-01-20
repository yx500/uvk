#include "dynamicotcepsystem.h"
#include "tos_speedcalc.h"
#include "m_rc_gor_zkr.h"
#include "m_rc_gor_park.h"
#include "m_ris.h"
#include <qfileinfo.h>
#include <QtMath>

DynamicOtcepSystem::DynamicOtcepSystem(QObject *parent, TrackingOtcepSystem *TOS) : BaseWorker(parent)
{
    this->TOS=TOS;
}

void DynamicOtcepSystem::calculateXoffset(tos_OtcepData *o, const QDateTime &T,int sf)
{
    auto otcep=o->otcep;
    if (sf==0){
        if (otcep->RCS!=nullptr){
            if (o->dos_dt_RCS_XOFFSET.isValid()){
                qreal xoffset=otcep->STATE_D_RCS_XOFFSET();
                if (otcep->STATE_V()!=_undefV_){
                    qint64 ms=o->dos_dt_RCS_XOFFSET.msecsTo(T);
                    if (ms>0){
                        xoffset=xoffset+tos_SpeedCalc::vt_len_m(otcep->STATE_V(),ms);
                    }
                    o->dos_dt_RCS_XOFFSET=T;
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
            if (o->dos_dt_RCF_XOFFSET.isValid()){
                qreal xoffset=otcep->STATE_D_RCF_XOFFSET();
                if (otcep->STATE_V()!=_undefV_){
                    qint64 ms=o->dos_dt_RCF_XOFFSET.msecsTo(T);
                    if (ms>0){
                        xoffset=xoffset+tos_SpeedCalc::vt_len_m(otcep->STATE_V(),ms);
                    }
                    o->dos_dt_RCF_XOFFSET=T;
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
    foreach (auto kzpt, TOS->l_kzpt) {

        // пересчитываем смещения
        bool recalc=false;
        if (!kzpt->trc->l_otceps.isEmpty()){
            auto num=kzpt->trc->l_otceps.last();
            if (TOS->mNUM2OD.contains(num)){
                auto otcep=TOS->mNUM2OD[num]->otcep;
                qreal end_x=kzpt->rc_park->STATE_KZP_D();
                if (otcep->RCF==kzpt->rc_park){
                    if (end_x!=otcep->STATE_D_RCF_XOFFSET()){
                        recalc=true;
                        otcep->setSTATE_D_RCF_XOFFSET(end_x);
                        otcep->setSTATE_D_RCS_XOFFSET(end_x+otcep->STATE_LEN());
                        end_x=otcep->STATE_D_RCS_XOFFSET();
                    }
                    otcep->setSTATE_V_KZP(kzpt->rc_park->STATE_KZP_V());
                }
                for (int i=kzpt->trc->l_otceps.size()-2;i>=0;i--){
                    auto num=kzpt->trc->l_otceps.last();
                    if (TOS->mNUM2OD.contains(num)){
                        auto o=TOS->mNUM2OD[num]->otcep;
                        // prognoz_KZP(o,T);// тут должен быть прогноз положения хвоста
                        if (recalc){
                            //                    if (dynamicStatistic!=nullptr){
                            //                        qint64 ms=o->tos->out_tp_t.msecsTo(T);
                            //                        qreal x=0;
                            //                        qreal v;
                            //                        dynamicStatistic->prognoz_ms_xv(o,o->RCF,ms,x,v);
                            //                        if (x>o->STATE_D_RCF_XOFFSET()){
                            //                            o->setSTATE_D_RCF_XOFFSET(x);
                            //                            o->setSTATE_D_RCS_XOFFSET(x+o->STATE_LEN());
                            //                        }
                            //                    }
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
    }
}

//void DynamicOtcepSystem::slot_otcep_rcsf_change(m_Otcep *otcep,int sf, m_RC *, m_RC *rcTo, QDateTime T, QDateTime )
//{
//    if (sf==0){
//        if ((otcep->STATE_V_RC()!=_undefV_)&&(rcTo)){
//            otcep->setSTATE_D_RCS_XOFFSET(0.001);
//            otcep->tos->dos_dt_RCS_XOFFSET=T;
//        } else {
//            otcep->setSTATE_D_RCS_XOFFSET(-1);
//            otcep->tos->dos_dt_RCS_XOFFSET=QDateTime();
//        }
//    }
//    if (sf==1){
//        if ((otcep->STATE_V_RC()!=_undefV_)&&(rcTo)){
//            otcep->setSTATE_D_RCF_XOFFSET(0.001);
//            otcep->tos->dos_dt_RCF_XOFFSET=T;
//        } else {
//            otcep->setSTATE_D_RCF_XOFFSET(-1);
//            otcep->tos->dos_dt_RCF_XOFFSET=QDateTime();
//        }

//    }

//}



void DynamicOtcepSystem::work(const QDateTime &T)
{

    foreach (auto *o, TOS->lo) {
        if ((o->otcep->RCF)&&(qobject_cast<m_RC_Gor_Park*>(o->otcep->RCF)!=nullptr)){
            // кзп

            continue;
        }
        if ((o->otcep->RCS)&&(qobject_cast<m_RC_Gor_ZKR*>(o->otcep->RCS)!=nullptr)){
            // zkr
            continue;
        }
        if ((o->otcep->RCF)&&(qobject_cast<m_RC_Gor_ZKR*>(o->otcep->RCF)!=nullptr)){
            // zkr
            calculateXoffset(o,T,0);
            continue;
        }
        calculateXoffset(o,T,0);
        calculateXoffset(o,T,1);

    }
    calculateXoffsetKzp(T);

}

void DynamicOtcepSystem::resetStates()
{
    BaseWorker::resetStates();
    foreach (auto *o, TOS->lo) {
        o->dos_RCS=nullptr;
        o->dos_RCF=nullptr;
        o->otcep->setSTATE_D_RCS_XOFFSET(-1);
        o->otcep->setSTATE_D_RCF_XOFFSET(-1);
        o->dos_dt_RCS_XOFFSET=QDateTime();
        o->dos_dt_RCF_XOFFSET=QDateTime();
    }
}
