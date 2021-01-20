#include "tos_otcepdata.h"

#include "trackingotcepsystem.h"
#include "tos_speedcalc.h"
#include <QMetaProperty>
#include <QtMath>

tos_OtcepData::tos_OtcepData(TrackingOtcepSystem *parent, m_Otcep *otcep) : BaseObject(parent)
{
    this->TOS=parent;
    this->otcep=otcep;

    setObjectName(QString("OtcepData %1").arg(otcep->NUM()));
}

void tos_OtcepData::resetStates()
{
    otcep->resetStates();
    resetTracking();


}

void tos_OtcepData::resetTracking()
{
    //снимаем только параметры TOS

    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
    otcep->setSTATE_MAR_F(0);
    otcep->setSTATE_ZKR_PROGRESS(false);
    otcep->setSTATE_KZP_OS(m_Otcep::kzpUnknow);
    otcep->setSTATE_V(_undefV_);
    otcep->setSTATE_V_RC(_undefV_);
    otcep->setSTATE_V_ARS(_undefV_);
    otcep->setSTATE_V_KZP(_undefV_);
    otcep->setSTATE_V_DISO(_undefV_);

    otcep->RCS=nullptr;
    otcep->RCF=nullptr;
    otcep->vBusyRc.clear();

    dos_RCS=nullptr;
    dos_RCF=nullptr;
    _stat_kzp_d=0;
    _stat_kzp_t=QDateTime();
//    out_tp_t=QDateTime();;
//    out_tp_rc=nullptr;
//    out_tp_v=_undefV_;

    setSTATE_V_RCF_IN(_undefV_);
    setSTATE_V_RCF_OUT(_undefV_);
}

void tos_OtcepData::setOtcepSF(int sf, m_RC *rcTo, int d, const QDateTime &T, bool bnorm)
{

    //скорость по РЦ считаем только в прямом направлении&=?
    qreal v_rc=_undefV_;
    if ((bnorm==_normal_track)&&(d==_forw)){
        m_RC* rcFrom=nullptr;
        QDateTime drFrom;
        if (sf==0){
            if ((rcTo!=nullptr)&&(rcTo->next_rc[1]==otcep->RCS)) { rcFrom=otcep->RCS;drFrom=dtRCS;}
        }
        if (sf==1){
            if ((rcTo!=nullptr)&&(rcTo->next_rc[1]==otcep->RCF)) { rcFrom=otcep->RCF;drFrom=dtRCF;}
        }
        if ((rcFrom!=nullptr)&&(drFrom.isValid())){
            qint64 ms=drFrom.msecsTo(T);
            v_rc=tos_SpeedCalc::calcV(ms,rcFrom->LEN());
        }
    }
    if (sf==0) {
        otcep->RCS=rcTo;
        if (bnorm) dtRCS=T; else dtRCS=QDateTime();
        if (d==0) otcep->setSTATE_D_RCS_XOFFSET(0);
    }
    if (sf==1) {
        otcep->RCF=rcTo;
        if (bnorm) dtRCF=T; else dtRCF=QDateTime();
    }
    otcep->setBusyRC();

    otcep->setSTATE_DIRECTION(d);
    otcep->setSTATE_V_RC(v_rc);

    if (sf==0) {
        if (d==0) {
            otcep->setSTATE_D_RCS_XOFFSET(0);
        } else {
            if (rcTo!=nullptr) otcep->setSTATE_D_RCS_XOFFSET(rcTo->LEN());
        }
        if ((!bnorm) && (rcTo!=nullptr)) otcep->setSTATE_D_RCS_XOFFSET(rcTo->LEN());
    }

    if (sf==1) {
        if (d==0) {
            otcep->setSTATE_D_RCF_XOFFSET(0);
        } else {
            if (rcTo!=nullptr) otcep->setSTATE_D_RCF_XOFFSET(rcTo->LEN());
        }
        if (!bnorm) otcep->setSTATE_D_RCF_XOFFSET(0);
    }

}


qreal tos_OtcepData::STATE_V() const
{
    if (otcep->STATE_V_ARS()!=_undefV_) return otcep->STATE_V_ARS();
    if (otcep->STATE_V_KZP()!=_undefV_) return otcep->STATE_V_KZP();
    if (otcep->STATE_V_DISO()!=_undefV_) return otcep->STATE_V_DISO();
    return otcep->STATE_V_RC();
}

void tos_OtcepData::updateV_RC(const QDateTime &T)
{
    // обнуляем скорость если голова не двигается
    if ((!dtRCS.isValid())&&(!dtRCF.isValid())){
        otcep->setSTATE_V_RC(_undefV_);
        return;
    }
    qreal Vrcs=otcep->STATE_V_RC();
    qreal Vrcf=otcep->STATE_V_RC();
    if (otcep->RCS==nullptr) Vrcs=_undefV_;
    if (otcep->RCF==nullptr) Vrcf=_undefV_;
    if ((Vrcs==_undefV_) &&(Vrcf==_undefV_)){
        otcep->setSTATE_V_RC(_undefV_);
        return;
    }
    if (dtRCS.isValid()){
        if ((otcep->RCS)&&(otcep->RCS->next_rc[0]!=nullptr)){
            qint64 ms=dtRCS.msecsTo(T);
            if (ms>=tos_SpeedCalc::calcMs(1.,otcep->RCS->LEN())) { // 1km/h
                Vrcs=0;
            }
        }

    }
    if (dtRCF.isValid()){
        if ((otcep->RCF)&&(otcep->RCF->next_rc[1]!=nullptr)){
            qint64 ms=dtRCF.msecsTo(T);
            if (ms>=tos_SpeedCalc::calcMs(1.,otcep->RCF->LEN())) { // 1km/h
                Vrcf=0;
            }
        }
    }
    if ((Vrcs==0) &&(Vrcf==0)){
        otcep->setSTATE_V_RC(0);
    }
}

//void tos_OtcepData::calcLenByRc()
//{
//    qreal l=0;
//    foreach (auto rc, otcep->vBusyRc) {
//        if (
//                (rc->LEN()!=0) &&
//                (rc->next_rc[0]!=nullptr) &&
//                (rc->next_rc[1]!=nullptr) &&
//                (rc->rcs->useRcTracking)&&
//                (rc->next_rc[0]->rcs->useRcTracking)
//                /*(qobject_cast<m_RC_Gor_Park*>(rc)==nullptr) &&
//                                                (qobject_cast<m_RC_Gor_Park*>(rc->next_rc[0])==nullptr) &&
//                                                (qobject_cast<m_RC_Gor_ZKR*>(rc)==nullptr)*/)
//        {
//            l=l+rc->LEN();
//        } else
//        {
//            return;
//        }
//    }
//    //    if ((otcep->STATE_LEN_BY_RC_MIN()==0)||(l<otcep->STATE_LEN_BY_RC_MIN()))
//    //        otcep->setSTATE_LEN_BY_RC_MIN(l);
//    //    if ((otcep->STATE_LEN_BY_RC_MAX()==0)||(l>otcep->STATE_LEN_BY_RC_MAX()))
//    //        otcep->setSTATE_LEN_BY_RC_MAX(l);
//}




