#include "tos_otcepdata.h"

#include "trackingotcepsystem.h"
#include "tos_speedcalc.h"
#include <QMetaProperty>
#include <QtMath>

tos_OtcepData::tos_OtcepData(TrackingOtcepSystem *parent, m_Otcep *otcep) : BaseObject(parent)
{
    this->TOS=parent;
    this->otcep=otcep;
    otcep->tos=this;

    setObjectName(QString("OtcepData %1").arg(otcep->NUM()));
}

void tos_OtcepData::resetStates()
{
    //otcep->resetStates();
    //снимаем только параметры TOS
    otcep->setSTATE_CHANGE_COUNTER(0);
    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
    otcep->setSTATE_MAR_F(0);
    otcep->setSTATE_DIRECTION(0);
    otcep->setSTATE_NAGON(0);
    otcep->setSTATE_ERROR(0);

    otcep->setSTATE_ZKR_PROGRESS(false);
    otcep->setSTATE_ZKR_VAGON_CNT(0);
    otcep->setSTATE_ZKR_OSY_CNT(0);
    otcep->setSTATE_ZKR_BAZA(0);

    otcep->setSTATE_KZP_OS(m_Otcep::kzpUnknow);

    otcep->setSTATE_V(_undefV_);
    otcep->setSTATE_V_RC(_undefV_);
    otcep->setSTATE_V_ARS(_undefV_);
    otcep->setSTATE_V_KZP(_undefV_);
    otcep->setSTATE_V_DISO(_undefV_);

    for (int i=0;i<3;i++){
        otcep->setSTATE_V_INOUT(0,i,_undefV_);
        otcep->setSTATE_V_INOUT(1,i,_undefV_);
        otcep->setSTATE_OT_RA(0,i,0);
        otcep->setSTATE_OT_RA(1,i,0);
    }

    otcep->RCS=nullptr;
    otcep->RCF=nullptr;
    otcep->vBusyRc.clear();

    dos_RCS=nullptr;
    dos_RCF=nullptr;
    _stat_kzp_d=0;
    _stat_kzp_t=QDateTime();
    out_tp_t=QDateTime();;
    out_tp_rc=nullptr;
    out_tp_v=_undefV_;

    setSTATE_V_RCF_IN(_undefV_);
    setSTATE_V_RCF_OUT(_undefV_);

}

void tos_OtcepData::updateOtcepParams(const QDateTime &T)
{
    if (TOS->mRc2Zam.contains(otcep->RCS)){
        m_Zam *zam=TOS->mRc2Zam[otcep->RCS];
        int n=zam->NTP();
        if ((zam->TIPZM()==1) &&(otcep->STATE_V_INOUT(0,n)==_undefV_)) otcep->setSTATE_V_INOUT(0,n,zam->ris()->STATE_V());
    }
    if (TOS->mRc2Zam.contains(otcep->RCF)){
        m_Zam *zam=TOS->mRc2Zam[otcep->RCF];
        int n=zam->NTP();
        otcep->tos->out_tp_t=T;
        otcep->tos->out_tp_rc=otcep->RCF;
        otcep->tos->out_tp_v=zam->ris()->STATE_V();
        if ((otcep->STATE_V_INOUT(1,n)==_undefV_)) otcep->setSTATE_V_INOUT(1,n,zam->ris()->STATE_V());

    }
    qreal Vars=_undefV_;
    foreach (auto rc, otcep->vBusyRc) {
        if (!TOS->mRc2Zam.contains(rc)) continue;
        m_Zam *zam=TOS->mRc2Zam[rc];
        int n=zam->NTP();
        if (zam->STATE_STUPEN()>0){
            if (zam->STATE_STUPEN()>otcep->STATE_OT_RA(0,n)) otcep->setSTATE_OT_RA(0,n,zam->STATE_STUPEN());
        }
        if (zam->STATE_A()!=0){
            otcep->setSTATE_OT_RA(1,n,zam->STATE_A());
        }
        if (TOS->mRc2Ris.contains(rc)){
            m_RIS *ris=TOS->mRc2Ris[rc];
            if (ris->controllerARS()->isValidState()){
                Vars=ris->STATE_V();
                if (Vars<1.3) Vars=0;
            }
        }

    }
    if ((qFabs(otcep->STATE_V_ARS()-Vars)>=0.4)) {
        otcep->setSTATE_V_ARS(Vars);
    }
    m_RC_Gor*rc=qobject_cast<m_RC_Gor*>(otcep->RCS);
    if ((rc==nullptr)||
            ((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)&&
             ((otcep->STATE_MAR()<rc->MINWAY())||(otcep->STATE_MAR()>rc->MAXWAY()))
             )
            )otcep->setSTATE_ERROR(true); else otcep->setSTATE_ERROR(false);
    int marf=0;
    while (rc!=nullptr){
        if (rc->MINWAY()==rc->MAXWAY()){
            marf=rc->MINWAY();
            break;
        }
        rc=qobject_cast<m_RC_Gor*>(rc->next_rc[0]);
    }
    otcep->setSTATE_MAR_F(marf);

    m_RC_Gor_ZKR *rc_zkr=qobject_cast<m_RC_Gor_ZKR*>(otcep->RCS);
    if (rc_zkr!=nullptr) otcep->setSTATE_PUT_NADVIG(rc_zkr->PUT_NADVIG());

    otcep->setSTATE_V(STATE_V());


    otcep->tos->calcLenByRc();
}

void tos_OtcepData::checkOtcepComplete()
{
    if (otcep->RCS==nullptr){
        otcep->RCF=nullptr;
        return;
    }
    // проверяем что хвост достижим
    bool backExists=false;
    if ((otcep->RCS!=nullptr)&&(otcep->RCF!=nullptr)){
        m_RC *RC=otcep->RCS;
        int grafSize=0;
        while (RC!=nullptr){
            if (RC==otcep->RCF) {
                backExists=true;
                break;
            }
            RC=RC->getNextRCpolcfb(1);
            grafSize++;
            if (grafSize>80) break;
        }
    }
    if (!backExists){
        // до последней занятости
        //otceps->clearOtcep_l_rc(otcep);
        int grafSize=0;
        otcep->RCF=otcep->RCS;
        //otcep->setSTATE_LEN_BY_RC(0);

        m_RC *RC=otcep->RCS->getNextRCpolcfb(1);
        while ((RC!=nullptr)&&(RC->STATE_BUSY()==MVP_Enums::free)){
            otcep->RCF=RC;
            RC=RC->getNextRCpolcfb(1);
            grafSize++;
            if (grafSize>80) {
                otcep->RCF=otcep->RCS;
                break;
            }
        }
        otcep->setBusyRC();
        TOS->updateOtcepsOnRc();
    }
}

void tos_OtcepData::setOtcepSF(m_RC *rcs, m_RC *rcf,const QDateTime &T)
{
    if ((otcep->RCS!=rcs)&&(otcep->RCF!=rcf)){
        emit TOS->otcep_rcsf_change(otcep,0,otcep->RCS,rcs,T,QDateTime());
        emit TOS->otcep_rcsf_change(otcep,1,otcep->RCF,rcf,T,QDateTime());
        otcep->RCS=rcs;
        otcep->RCF=rcf;
        dtRCS=QDateTime();
        dtRCF=QDateTime();
        otcep->setSTATE_V_RC(_undefV_);
        otcep->setBusyRC();
        TOS->updateOtcepsOnRc();
    } else
        if (otcep->RCS!=rcs){
            if ((rcs!=nullptr)&&(rcs->next_rc[1]==otcep->RCS)){
                qint64 ms=dtRCS.msecsTo(T);
                qreal V=tos_SpeedCalc::calcV(ms,otcep->RCS->LEN());
                otcep->setSTATE_V_RC(V);
                emit TOS->otcep_rcsf_change(otcep,0,otcep->RCS,rcs,T,dtRCS);
                dtRCS=T;
            } else {
                dtRCS=QDateTime();
                otcep->setSTATE_V_RC(_undefV_);
                emit TOS->otcep_rcsf_change(otcep,0,otcep->RCS,rcs,T,dtRCS);
            }
            otcep->RCS=rcs;
            otcep->setBusyRC();
            TOS->updateOtcepsOnRc();
        } else
            if (otcep->RCF!=rcf){
                if ((rcf!=nullptr)&&(rcf->next_rc[1]==otcep->RCF)){
                    qint64 ms=dtRCF.msecsTo(T);
                    qreal V=tos_SpeedCalc::calcV(ms,otcep->RCF->LEN());
                    otcep->setSTATE_V_RC(V);
                    emit TOS->otcep_rcsf_change(otcep,1,otcep->RCF,rcf,T,dtRCF);
                    dtRCF=T;
                } else {
                    dtRCF=QDateTime();
                    emit TOS->otcep_rcsf_change(otcep,1,otcep->RCF,rcf,T,dtRCF);
                }
                otcep->RCF=rcf;
                otcep->setBusyRC();
                TOS->updateOtcepsOnRc();
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
    if ((!otcep->tos->dtRCS.isValid())&&(!otcep->tos->dtRCF.isValid())){
        otcep->setSTATE_V_RC(_undefV_);
        return;
    }
    qreal Vrcs=otcep->STATE_V_RC();
    qreal Vrcf=otcep->STATE_V_RC();
    if ((otcep->RCS==nullptr)||(!otcep->RCS->rcs->useRcTracking)) Vrcs=_undefV_;
    if ((otcep->RCF==nullptr)||(!otcep->RCF->rcs->useRcTracking)) Vrcf=_undefV_;
    if ((Vrcs==_undefV_) &&(Vrcf==_undefV_)){
        otcep->setSTATE_V_RC(_undefV_);
        return;
    }
    if (otcep->tos->dtRCS.isValid()){
        if ((otcep->RCS)&&(otcep->RCS->next_rc[0]!=nullptr)){
            qint64 ms=otcep->tos->dtRCS.msecsTo(T);
            if (ms>=tos_SpeedCalc::calcMs(1.,otcep->RCS->LEN())) { // 1km/h
                Vrcs=0;
            }
        }

    }
    if (otcep->tos->dtRCF.isValid()){
        if ((otcep->RCF)&&(otcep->RCF->next_rc[1]!=nullptr)){
            qint64 ms=otcep->tos->dtRCF.msecsTo(T);
            if (ms>=tos_SpeedCalc::calcMs(1.,otcep->RCF->LEN())) { // 1km/h
                Vrcf=0;
            }
        }
    }
    if ((Vrcs==0) &&(Vrcf==0)){
        otcep->setSTATE_V_RC(0);
    }
}

void tos_OtcepData::calcLenByRc()
{
    qreal l=0;
    foreach (auto rc, otcep->vBusyRc) {
        if (
                (rc->LEN()!=0) &&
                (rc->next_rc[0]!=nullptr) &&
                (rc->next_rc[1]!=nullptr) &&
                (rc->rcs->useRcTracking)&&
                (rc->next_rc[0]->rcs->useRcTracking)
                /*(qobject_cast<m_RC_Gor_Park*>(rc)==nullptr) &&
                                (qobject_cast<m_RC_Gor_Park*>(rc->next_rc[0])==nullptr) &&
                                (qobject_cast<m_RC_Gor_ZKR*>(rc)==nullptr)*/)
        {
            l=l+rc->LEN();
        } else
        {
            return;
        }
    }
    //    if ((otcep->STATE_LEN_BY_RC_MIN()==0)||(l<otcep->STATE_LEN_BY_RC_MIN()))
    //        otcep->setSTATE_LEN_BY_RC_MIN(l);
    //    if ((otcep->STATE_LEN_BY_RC_MAX()==0)||(l>otcep->STATE_LEN_BY_RC_MAX()))
    //        otcep->setSTATE_LEN_BY_RC_MAX(l);
}

void tos_OtcepData::state2buffer()
{

}



void tos_OtcepData::setCurrState()
{

    otcep->tos->rc_tracking_comleted[0]=false;
    otcep->tos->rc_tracking_comleted[1]=false;
}



