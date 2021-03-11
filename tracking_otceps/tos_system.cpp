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

tos_OtcepData *tos_System::getNewOtcep(tos_Rc *trc,int drobl)
{
    if (iGetNewOtcep!=nullptr){
        int num=iGetNewOtcep->getNewOtcep(trc->rc,drobl);
        if (num>0){
            return mNUM2OD[num];
        }
    }
    return nullptr;
}

int tos_System::resetOtcep2prib(int num)
{
    if (iGetNewOtcep!=nullptr){
        num=iGetNewOtcep->resetOtcep2prib(num);
    }
    return num;
}

int tos_System::nerascep(int num)
{
    if (iGetNewOtcep!=nullptr){
        num=iGetNewOtcep->nerascep(num);
    }
    return num;
}

void tos_System::work(const QDateTime &)
{
    if (!FSTATE_ENABLED) return;
}

void tos_System::updateOtcepsParams(const QDateTime &T)
{

    // признак на зкр
    m_Otcep *otcep_on_zkr=nullptr;
    foreach (auto o, lo) {
        auto otcep=o->otcep;
        bool inzkr=false;
        otcep->setSTATE_ZKR_S_IN(0);
        if (mRc2Zkr.contains(otcep->RCS)){
            inzkr=true;
            otcep_on_zkr=otcep;
            otcep->setSTATE_PUT_NADVIG(mRc2Zkr[otcep->RCS]->PUT_NADVIG());
            otcep->setSTATE_ZKR_S_IN(1);
        }
        if (mRc2Zkr.contains(otcep->RCF)){
            inzkr=true;
            otcep->setSTATE_PUT_NADVIG(mRc2Zkr[otcep->RCF]->PUT_NADVIG());

        }
        otcep->setSTATE_ZKR_PROGRESS(inzkr);
    }
    if (otcep_on_zkr!=nullptr) otcep_on_zkr->setSTATE_ZKR_S_IN(1);



    foreach (auto o, lo) {
        auto otcep=o->otcep;
        if (!otcep->STATE_ENABLED()) return;
        if ((otcep->RCS==nullptr)&&(otcep->RCF==nullptr)&&(otcep->STATE_LOCATION()==m_Otcep::locationOnPrib)) return;
        if ((otcep->RCS==nullptr)&&(otcep->RCF==nullptr)){
            otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
            return;
        }
        int locat=m_Otcep::locationOnSpusk;

        // признак на зкр


        // KZP
        m_RC_Gor_Park * rc_park=qobject_cast<m_RC_Gor_Park *>(otcep->RCF);
        if (rc_park!=nullptr){
            locat=m_Otcep::locationOnPark;
        }


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
        qreal Vars=_undefV_;
        foreach (auto rc, otcep->vBusyRc) {
            if (!mRc2Zam.contains(rc)) continue;
            m_Zam *zam=mRc2Zam[rc];
            int n=zam->NTP();
            // режим торм
            if (zam->STATE_STUPEN()>0){
                if (zam->STATE_STUPEN()>otcep->STATE_OT_RA(0,n)) otcep->setSTATE_OT_RA(0,n,zam->STATE_STUPEN());
            }
            if (zam->STATE_A()!=0){
                otcep->setSTATE_OT_RA(1,n,zam->STATE_A());
            }
            if (mRc2Ris.contains(rc)){
                m_RIS *ris=mRc2Ris[rc];
                if (ris->controllerARS()->isValidState()){
                    Vars=ris->STATE_V();
                    if (Vars<1.3) Vars=0;
                }
            }

        }
        if ((qFabs(otcep->STATE_V_ARS()-Vars)>=0.4)) {
            otcep->setSTATE_V_ARS(Vars);
        }
        // маршрут
        m_RC_Gor*rc=qobject_cast<m_RC_Gor*>(otcep->RCS);
        if ((rc==nullptr)||
                (((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)||(otcep->STATE_LOCATION()==m_Otcep::locationOnPark))&&
                 (otcep->STATE_MAR()!=0)&&
                 ((otcep->STATE_MAR()<rc->MINWAY())||(otcep->STATE_MAR()>rc->MAXWAY()))
                 )
                )otcep->setSTATE_ERROR(true); else otcep->setSTATE_ERROR(false);
        if ((rc==nullptr)&&(rc->MINWAY()==rc->MAXWAY())&&(rc->MINWAY()!=0)) otcep->setSTATE_MAR_R(1);else otcep->setSTATE_MAR_R(0);
        int marf=0;
        while (rc!=nullptr){
            if (rc->MINWAY()==rc->MAXWAY()){
                marf=rc->MINWAY();
                break;
            }
            rc=qobject_cast<m_RC_Gor*>(rc->next_rc[_forw]);
        }
        otcep->setSTATE_MAR_F(marf);



        otcep->setSTATE_LOCATION(locat);

//        // финализируем скорость отцепов
//        if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk){
//            o->updateV_RC(T);
//        }
        otcep->setSTATE_V(o->STATE_V());

        // порядковый на рц
        int nn=0;
        foreach (auto o2, lo) {
            auto otcep2=o2->otcep;
            if ((otcep2->RCS!=nullptr)&&(otcep2->RCS==otcep->RCS)){
                if (otcep2->NUM()>otcep->NUM()) nn++;
            }
        }
        otcep->setSTATE_D_ORDER_RC(nn);

    }

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
