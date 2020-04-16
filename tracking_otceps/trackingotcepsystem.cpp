#include "trackingotcepsystem.h"

#include <qdebug.h>
#include <assert.h>
#include "baseobjecttools.h"

#include "tos_speedcalc.h"
#include "m_otceps.h"
#include "m_zam.h"


TrackingOtcepSystem::TrackingOtcepSystem(QObject *parent,ModelGroupGorka *modelGorka) :
    BaseWorker(parent)
{
    this->modelGorka=modelGorka;
    makeWorkers(modelGorka);
}

QList<BaseWorker *> TrackingOtcepSystem::makeWorkers(ModelGroupGorka *O)
{
    this->modelGorka=O;
    QList<BaseWorker *> l;
    // собираем все отцепы
    auto lotceps=modelGorka->findChildren<m_Otceps *>();
    if (!lotceps.isEmpty()) {
        otceps=lotceps.first();
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
}


void TrackingOtcepSystem::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;
    bool self=true;
    if (self){
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




    } else {

        //        foreach (auto w, lw) {
        //            // сброс при след роспуске
        //            if (w->otcep->stored_Descr.Id!=w->Descr_Id) {
        //                w->resetStates();
        //                w->Descr_Id=w->otcep->stored_Descr.Id;
        //                foreach (tos_KzpTracking *kzpt, l_kzpt) {
        //                    kzpt->rc_park->removeOtcep(w->otcep);
        //                }

        //            }
        //        }
        //        foreach (auto w, lw) {
        //            // запоминаем пред
        //            w->_RCS=w->otcep->RCS;
        //            w->_RCF=w->otcep->RCF;

        //            auto otcep=w->otcep;
        //            bool s_in_park=(qobject_cast<m_RC_Gor_Park *>(otcep->RCS)!=nullptr);
        //            bool f_in_park=(qobject_cast<m_RC_Gor_Park *>(otcep->RCF)!=nullptr);
        //            // за парк отвечает tos_KzpTracking

        //            if (!s_in_park || !f_in_park) {
        //                otcep->setSTATE_V_KZP(_undefV_);

        //                if ((otcep->stored_Descr.num==0)||(otcep->stored_Descr.end_slg!=0)){
        //                    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
        //                    w->resetStates();
        //                    continue;
        //                }
        //            }
        //            if (!s_in_park){
        //                otcep->setSTATE_LOCATION(m_Otcep::locationOnSpusk);
        //                otcep->setOtcepSF(otcep->descr_RCS,otcep->RCF);
        //            }
        //            if (!f_in_park) {
        //                otcep->setSTATE_LOCATION(m_Otcep::locationOnSpusk);
        //                otcep->setOtcepSF(w->otcep->RCS,w->otcep->descr_RCF);
        //            }

        //            if (w->_RCS!=w->otcep->RCS) {
        //                if ((w->_RCS) && (w->otcep->RCS) && (w->otcep->RCS==w->_RCS->next_rc[1])) w->otcep->setSTATE_DIRECTION(1); else w->otcep->setSTATE_DIRECTION(0);
        //                if (w->otcep->STATE_DIRECTION()==0) w->otcep->setSTATE_RCS_XOFFSET(0); else w->otcep->setSTATE_RCS_XOFFSET(w->otcep->RCS->LEN());
        //                w->dtSTATE_RCS_XOFFSET_C=T;
        //                w->add_route(_pOtcepStart,w->otcep->RCS,T);
        //                w->setSTATE_V_RCS(tos_SpeedCalc::calcOtcepV_RC(w,_pOtcepStart));
        //                w->otcep->setSTATE_V_RC(w->STATE_V_RCS());
        //            }
        //            if (w->_RCF!=w->otcep->RCF) {
        //                if ((w->_RCF) && (w->otcep->RCF) && (w->otcep->RCF==w->_RCF->next_rc[1])) w->otcep->setSTATE_DIRECTION(1); else w->otcep->setSTATE_DIRECTION(0);
        //                if (w->otcep->STATE_DIRECTION()==0) w->otcep->setSTATE_RCF_XOFFSET(0); else w->otcep->setSTATE_RCF_XOFFSET(w->otcep->RCS->LEN());
        //                w->dtSTATE_RCF_XOFFSET_C=T;
        //                w->add_route(_pOtcepEnd,w->otcep->RCF,T);
        //                w->setSTATE_V_RCF(tos_SpeedCalc::calcOtcepV_RC(w,_pOtcepEnd));
        //                w->otcep->setSTATE_V_RC(w->STATE_V_RCF());
        //                calcLenByRc(w->otcep);
        //            }
        //        }
    }


    // финализируем скорость отцепов +V ARS
    foreach (auto otcep, lo) {
        if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
            updateV_RC(otcep,T);
    }


    // выставляемпараметра отцепа
    foreach (auto otcep, lo) {
        if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
            updateOtcepParams(otcep);
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

void TrackingOtcepSystem::updateOtcepParams(m_Otcep *otcep)
{
    if (mRc2Zam.contains(otcep->RCS)){
        m_Zam *zam=mRc2Zam[otcep->RCS];
        if ((zam->NTP()==1) &&(otcep->STATE_V_IN_1()==_undefV_)) otcep->setSTATE_V_IN_1(zam->ris()->STATE_V());
        if ((zam->NTP()==2) &&(otcep->STATE_V_IN_2()==_undefV_)) otcep->setSTATE_V_IN_2(zam->ris()->STATE_V());
        if ((zam->NTP()==3) &&(otcep->STATE_V_IN_3()==_undefV_)) otcep->setSTATE_V_IN_3(zam->ris()->STATE_V());
    }
    if (mRc2Zam.contains(otcep->RCF)){
        m_Zam *zam=mRc2Zam[otcep->RCF];
        if ((zam->NTP()==1) &&(otcep->STATE_V_OUT_1()==_undefV_)) otcep->setSTATE_V_OUT_1(zam->ris()->STATE_V());
        if ((zam->NTP()==2) &&(otcep->STATE_V_OUT_2()==_undefV_)) otcep->setSTATE_V_OUT_2(zam->ris()->STATE_V());
        if ((zam->NTP()==3) &&(otcep->STATE_V_OUT_3()==_undefV_)) otcep->setSTATE_V_OUT_3(zam->ris()->STATE_V());
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
