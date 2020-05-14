#include "trackingotcepsystem.h"

#include <qdebug.h>
#include <assert.h>
#include "baseobjecttools.h"
#include "mvp_system.h"

#include "tos_speedcalc.h"
#include "m_otceps.h"
#include "m_zam.h"


TrackingOtcepSystem::TrackingOtcepSystem(QObject *parent,ModelGroupGorka *modelGorka,int trackingType) :
    BaseWorker(parent)
{
    this->modelGorka=modelGorka;
    this->trackingType=trackingType;
    makeWorkers(modelGorka);
}
void TrackingOtcepSystem::disableBuffers()
{
    foreach (auto *otcep, lo) {
        otcep->setSIGNAL_DATA( otcep->SIGNAL_DATA().innerUse());
    }
    for (int i=0;i<MaxVagon;i++){
        otceps->chanelVag[i]->static_mode=true;
    }
}

QList<BaseWorker *> TrackingOtcepSystem::makeWorkers(ModelGroupGorka *O)
{
    this->modelGorka=O;
    QList<BaseWorker *> l;
    // собираем все отцепы
    auto lotceps=modelGorka->findChildren<m_Otceps *>();
    if (!lotceps.isEmpty()) {
        otceps=lotceps.first();
        otceps->disableUpdateStates=true;
        lo=otceps->findChildren<m_Otcep *>();
        foreach (auto *otcep, lo) {
            tos_OtcepData *w=new tos_OtcepData(this,otcep);
            // отключаем собственную обработку
            otcep->disableUpdateStates=true;
            otcep->addTagObject(w,27);
        }
    }
    // собираем все РЦ
    auto l_rc=modelGorka->findChildren<m_RC*>();
    foreach (auto rc, l_rc) {


        m_RC_Gor_Park *rc_park=qobject_cast<m_RC_Gor_Park*>(rc);
        if (rc_park!=nullptr){
            tos_KzpTracking *kzpTracking=new tos_KzpTracking (this,rc_park);
            l_kzpt.push_back(kzpTracking);
            l_rct.push_back(kzpTracking);
            continue;
        }
        m_RC_Gor_ZKR *rc_zkr=qobject_cast<m_RC_Gor_ZKR*>(rc);
        if (rc_zkr!=nullptr){
            tos_ZkrTracking *zkrTracking=new tos_ZkrTracking (this,rc_zkr,otceps,modelGorka);
            l_zkrt.push_back(zkrTracking);
            l_rct.push_back(zkrTracking);
            continue;
        }
        tos_RcTracking *rct=new tos_RcTracking(this,rc);


        l_rct.push_back(rct);
    }
    // собираем замедлители для скорости входа выхода
    auto l_zam=modelGorka->findChildren<m_Zam*>();
    foreach (auto zam, l_zam) {
        mRc2Zam.insert(zam->rc(),zam);
    }

    resetStates();
    return l;
}

void TrackingOtcepSystem::resetStates()
{
    otceps->resetStates();
    foreach (auto rct, l_rct) {
        rct->resetStates();
    }
    foreach (auto otcep, lo) {
        otcep->tos->resetStates();
    }
    foreach (auto *kzpt, l_kzpt) {
        kzpt->resetStates();
    }
    foreach (auto *zkrt, l_zkrt) {
        zkrt->resetStates();
    }


}

void TrackingOtcepSystem::state2buffer()
{
    foreach (auto rct, l_rct) {
        rct->state2buffer();
    }
    foreach (auto otcep, lo) {
        otcep->tos->state2buffer();
    }
    foreach (auto *kzpt, l_kzpt) {
        kzpt->state2buffer();
    }
    foreach (auto *zkrt, l_zkrt) {
        zkrt->state2buffer();
    }
    for (int i=0;i<MaxVagon;i++){
        if (otceps->TYPE_DESCR()==0){
            otceps->chanelVag[i]->A.resize(sizeof(otceps->vagons[i]));
            memcpy(otceps->chanelVag[i]->A.data(),&otceps->vagons[i],sizeof(otceps->vagons[i]));
        }
        if (otceps->TYPE_DESCR()==1){
            QVariantHash m=tSlVagon2Map(otceps->vagons[i]);
            QString S=MVP_System::QVariantHashToQString_str(m);
            otceps->chanelVag[i]->A=S.toUtf8();
        }
    }

}



void TrackingOtcepSystem::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;
    if (trackingType==0){
        // механизм таблицы переходов
        foreach (auto rct, l_rct) {
            rct->setCurrState();
        }
        foreach (auto otcep, lo) {
            otcep->tos->setCurrState();
        }

        // зкр
        foreach (tos_ZkrTracking *zkrt, l_zkrt) {
            zkrt->work(T);
        }

        // кзп
        foreach (tos_KzpTracking *kzpt, l_kzpt) {
            kzpt->work(T);
        }

        // следим по РЦ
        // двигаем нормально вперед голову
        foreach (auto rct, l_rct) {
            rct->workOtcep(0/*start*/,T,tos_normal);
        }
        // двигаем нормально вперед хвост
        foreach (auto rct, l_rct) {
            rct->workOtcep(1/*end*/,T,tos_normal);
        }
        // двигаем назад голову и другие исключения
        foreach (auto rct, l_rct) {
            rct->workOtcep(0/*start*/,T,tos_hard);
        }
        // двигаем назад хвост и другие исключения
        foreach (auto rct, l_rct) {
            rct->workOtcep(1/*end*/,T,tos_hard);
        }

        // проверяем отцепы
        foreach (auto otcep, lo) {
            checkOtcepComplete(otcep);
        }



        // финализируем скорость отцепов +V ARS
        foreach (auto otcep, lo) {
            if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
                updateV_RC(otcep,T);
        }


        // выставляем параметра отцепа
        foreach (auto otcep, lo) {
            if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
                updateOtcepParams(otcep,T);
        }

    } else {
        if ((_regim==ModelGroupGorka::regimStop)&&(modelGorka->STATE_REGIM()==ModelGroupGorka::regimRospusk)){
            foreach (auto otcep, lo) {
                if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib) otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                otcep->setSTATE_ENABLED(false);
                otcep->tos->setOtcepSF(nullptr,nullptr,T);
            }
        }
        _regim=modelGorka->STATE_REGIM();

        // следим по дескр +кзп
        foreach (auto otcep, lo) {
            otcep->update_descr();
        }



        // убираем заброшенные
        quint32 idrosp=0;
        if (lo.first()->STATE_ENABLED()){
            idrosp=lo.first()->STATE_ID_ROSP();
        }
        foreach (auto otcep, lo) {
            if ((!otcep->STATE_ENABLED())||(otcep->STATE_ID_ROSP()!=idrosp)){
                otcep->setSTATE_ENABLED(false);
                otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                otcep->tos->setOtcepSF(nullptr,nullptr,T);
            }
        }


        foreach (auto otcep, lo) {
            if (!otcep->STATE_ENABLED()) continue;
            bool s_in_park=(qobject_cast<m_RC_Gor_Park *>(otcep->RCS)!=nullptr);
            bool f_in_park=(qobject_cast<m_RC_Gor_Park *>(otcep->RCF)!=nullptr);
            if (!s_in_park) otcep->tos->setOtcepSF(otcep->descr_RCS,otcep->RCF,T);
            if (!f_in_park) otcep->tos->setOtcepSF(otcep->RCS,otcep->descr_RCF,T);
            if (f_in_park){
                if ((otcep->descr_RCF==nullptr) || (otcep->RCF==nullptr)||(otcep->descr_RCF!=otcep->RCF->next_rc[1])){
                    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                    otcep->tos->setOtcepSF(nullptr,nullptr,T);
                    continue;
                }
            }
            if (s_in_park){
                if ((otcep->descr_RCS==nullptr) || (otcep->RCS==nullptr)||(otcep->descr_RCS!=otcep->RCS->next_rc[1])){
                    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                    otcep->tos->setOtcepSF(nullptr,nullptr,T);
                    continue;
                }
            }
            m_Otcep::TOtcepLocation l=m_Otcep::locationUnknow;
            if (f_in_park) l=m_Otcep::locationOnPark;else
                if (s_in_park)l=m_Otcep::locationOnSpusk; else
                    if (otcep->stored_Descr.end_slg==0) {
                        if ((otcep->RCS!=nullptr)&&(otcep->RCF!=nullptr)) l=m_Otcep::locationOnSpusk;
                    }
            otcep->setSTATE_LOCATION(l);

        }
        // кзп
        foreach (tos_KzpTracking *kzpt, l_kzpt) {
            kzpt->work(T);
        }



        // финализируем скорость отцепов +V ARS
        foreach (auto otcep, lo) {
            if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
                updateV_RC(otcep,T);
        }
        // выставляем параметра отцепа
        foreach (auto otcep, lo) {
            if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
                updateOtcepParams(otcep,T);
        }

    }
}


void TrackingOtcepSystem::updateOtcepsOnRc()
{
    foreach (auto rct, l_rct) {
        foreach (auto otcep, lo) {
            if(otcep->vBusyRc.contains(rct->rc)){
                rct->addOtcep(otcep);
            } else {
                rct->removeOtcep(otcep);
            }
        }
    }
}

void TrackingOtcepSystem::checkOtcepComplete(m_Otcep *otcep)
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
    }
}

void TrackingOtcepSystem::updateOtcepParams(m_Otcep *otcep, const QDateTime &T)
{
    if (mRc2Zam.contains(otcep->RCS)){
        m_Zam *zam=mRc2Zam[otcep->RCS];
        if ((zam->NTP()==1) &&(otcep->STATE_V_IN_1()==_undefV_)) {otcep->setSTATE_V_IN_1(zam->ris()->STATE_V());}
        if ((zam->NTP()==2) &&(otcep->STATE_V_IN_2()==_undefV_)) {otcep->setSTATE_V_IN_2(zam->ris()->STATE_V());}
        if ((zam->NTP()==3) &&(otcep->STATE_V_IN_3()==_undefV_)) {otcep->setSTATE_V_IN_3(zam->ris()->STATE_V());}
    }
    if (mRc2Zam.contains(otcep->RCF)){
        m_Zam *zam=mRc2Zam[otcep->RCF];
        otcep->tos->out_tp_t=T;
        otcep->tos->out_tp_rc=otcep->RCF;
        otcep->tos->out_tp_v=zam->ris()->STATE_V();


        if ((zam->NTP()==1) &&(otcep->STATE_V_OUT_1()==_undefV_)) {otcep->setSTATE_V_OUT_1(zam->ris()->STATE_V());}
        if ((zam->NTP()==2) &&(otcep->STATE_V_OUT_2()==_undefV_)) {otcep->setSTATE_V_OUT_2(zam->ris()->STATE_V());}
        if ((zam->NTP()==3) &&(otcep->STATE_V_OUT_3()==_undefV_)) {otcep->setSTATE_V_OUT_3(zam->ris()->STATE_V());}

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
     if (rc_zkr!=nullptr) otcep->setSTATE_PUT_NADVIG(rc_zkr->PUTNADVIG());

    otcep->tos->calcLenByRc();

}
void TrackingOtcepSystem::updateV_RC(m_Otcep *otcep, const QDateTime &T)
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
            if (ms>=otcep->RCS->LEN()*3600) { // 1km/h
                Vrcs=0;
            }
        }

    }
    if (otcep->tos->dtRCF.isValid()){
        if ((otcep->RCF)&&(otcep->RCF->next_rc[1]!=nullptr)){
            qint64 ms=otcep->tos->dtRCF.msecsTo(T);
            if (ms>=otcep->RCF->LEN()*3600) { // 1km/h
                Vrcf=0;
            }
        }
    }
    if ((Vrcs==0) &&(Vrcf==0)){
        otcep->setSTATE_V_RC(0);
    }
}
