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
    otceps->disableUpdateStates=true;
    foreach (auto *otcep, lo) {
        otcep->setSIGNAL_DATA( otcep->SIGNAL_DATA().innerUse());
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
            otcep->tos->checkOtcepComplete();
        }





    } else {
        // следим по дескр +кзп
        if ((_regim==ModelGroupGorka::regimStop)&&(modelGorka->STATE_REGIM()==ModelGroupGorka::regimRospusk)){
            foreach (auto otcep, lo) {
                if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib) otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                otcep->setSTATE_ENABLED(false);
                otcep->tos->setOtcepSF(nullptr,nullptr,T);
            }
        }
        _regim=modelGorka->STATE_REGIM();


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
    }

    // финализируем скорость отцепов
    foreach (auto otcep, lo) {
        if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
            otcep->tos->updateV_RC(T);
    }


    // выставляем параметра отцепа
    foreach (auto otcep, lo) {
        if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)
            otcep->tos->updateOtcepParams(T);
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


