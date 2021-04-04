#include "tos_system.h"

#include <QtMath>

tos_System::tos_System(QObject *parent) :
    BaseWorker(parent)
{
    this->modelGorka=nullptr;
    iGetNewOtcep=nullptr;
}


void tos_System::makeWorkers(ModelGroupGorka *modelGorka)
{
    this->modelGorka=modelGorka;
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
        l_trc.push_back(trc);
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
    // собираем ЗКР
    l_zkr=modelGorka->findChildren<m_RC_Gor_ZKR*>();
    foreach (auto zkr, l_zkr) {
        mRc2Zkr.insert(zkr,zkr);
    }
    // собираем парк
    l_park=modelGorka->findChildren<m_RC_Gor_Park*>();
    foreach (auto p, l_park) {
        mRc2Park.insert(p,p);
    }
    // собираем упр стрелки
    l_strel_Y=modelGorka->findChildren<m_Strel_Gor_Y*>();
    resetStates();
}

void tos_System::resetStates()
{
    foreach (auto otcep, lo) {
        otcep->resetStates();
    }
    foreach (auto w, l_trc) {
        w->resetStates();
    }

}
QList<SignalDescription> tos_System::acceptOutputSignals()
{

    QList<SignalDescription> l;
    foreach (auto trc, l_trc) {
        l+=trc->acceptOutputSignals();
    }
    return l;
}
void tos_System::state2buffer()
{


    // а мы ничего не посылаем для этого есть OtcepsController
    //    foreach (auto otcep, lo) {
    //        otcep->tos->state2buffer();
    //    }
}

int tos_System::getNewOtcep(m_RC_Gor_ZKR*rc_zkr)
{
    if (iGetNewOtcep!=nullptr){
        return  iGetNewOtcep->getNewOtcep(rc_zkr);
    }
    return 0;
}

int tos_System::resetOtcep2prib(m_RC_Gor_ZKR*rc_zkr, int num )
{
    if (iGetNewOtcep!=nullptr){
        num=iGetNewOtcep->resetOtcep2prib(rc_zkr,num);
    }
    return 0;
}

int tos_System::exitOtcep(m_RC_Gor_ZKR *rc_zkr, int num)
{
    if (iGetNewOtcep!=nullptr){
        num=iGetNewOtcep->exitOtcep(rc_zkr,num);
    }
    return 0;
}



void tos_System::work(const QDateTime &)
{
    if (!FSTATE_ENABLED) return;
}

void tos_System::updateOtcepsParams(const QDateTime &)
{



        //        // скорость входа
        //        if (mRc2Zam.contains(otcep->RCS)){
        //            m_Zam *zam=mRc2Zam[otcep->RCS];
        //            int n=zam->NTP()-1;
        //            auto v=otcep->STATE_V_INOUT(0,n);
        //            auto v2=zam->ris()->STATE_V();
        //            if ((zam->TIPZM()==1) &&(otcep->STATE_V_INOUT(0,n)==_undefV_)&&(zam->ris()!=nullptr)&&(zam->ris()->STATE_V()!=_undefV_)) otcep->setSTATE_V_INOUT(0,n,zam->ris()->STATE_V());
        //        }
        //        // скорость выхода
        //        if (mRc2Zam.contains(otcep->RCF)){
        //            m_Zam *zam=mRc2Zam[otcep->RCF];
        //            int n=zam->NTP()-1;
        //            if ((zam->ris()!=nullptr)&&(zam->ris()->STATE_V()!=_undefV_)) otcep->setSTATE_V_INOUT(1,n,zam->ris()->STATE_V());

        //        }


}


void tos_System::resetTracking(int num)
{
    if (mNUM2OD.contains(num)){
        auto otcep=mNUM2OD[num];
        otcep->resetTracking();
    }

}

void tos_System::resetTracking()
{
    foreach (auto otcep,lo){
        otcep->resetTracking();
    }

}

tos_OtcepData *tos_System::otcep_by_num(int num)
{
    if (mNUM2OD.contains(num)) return mNUM2OD[num];
    return nullptr;
}
