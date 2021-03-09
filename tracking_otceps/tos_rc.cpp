#include "tos_rc.h"
#include "tos_system.h"

tos_Rc::tos_Rc(tos_System *TOS, m_RC *rc)
{

    this->TOS=TOS;
    this->rc=rc;
    tdso[0]=nullptr;
    tdso[1]=nullptr;
    l_os.reserve(128);
//    STATE_BUSY=MVP_Enums::busy_unknow;
//    useRcTracking=false;
}

void tos_Rc::work(const QDateTime &T)
{

    for (int d=0;d<2;d++)
        next_rc[d]=TOS->mRc2TRC[rc->next_rc[d]];

//    auto new_state=rc->STATE_BUSY();
//    // дребезг 11111110 10101 000000000
//    if (new_state!=STATE_BUSY){
//        if ((!time_STATE_BUSY.isValid()) ||
//                (time_STATE_BUSY.msecsTo(T)>500)){
//            STATE_BUSY=new_state;
//            time_STATE_BUSY=T;
//        }
//    }
    // снимаем ложные занятости
    if (STATE_ERR_KZ){
        if ((rc->next_rc[0]!=nullptr)&&(rc->STATE_BUSY()!=rc->next_rc[0]->STATE_BUSY()))
            STATE_ERR_KZ=false;
    }
    if ((STATE_ERR_LS)&&(rc->STATE_BUSY()==1)) STATE_ERR_LS=false;
    if ((STATE_ERR_LZ)&&(rc->STATE_BUSY()==0)) STATE_ERR_LZ=false;

//    if (STATE_CHECK_FREE_DB){
//        if (rc->STATE_BUSY()==1) STATE_CHECK_FREE_DB=false;
//        if ((rc->next_rc[0]!=nullptr)&&(rc->next_rc[0]->STATE_BUSY()==0)) STATE_CHECK_FREE_DB=false;
//        if ((rc->next_rc[1]!=nullptr)&&(rc->next_rc[1]->STATE_BUSY()==0)) STATE_CHECK_FREE_DB=false;
//    }

    set_v_dso(T);

}

void tos_Rc::resetStates()
{
//    STATE_BUSY=MVP_Enums::busy_unknow;
//    time_STATE_BUSY=QDateTime();
    STATE_ERR_LS=false;
    STATE_ERR_LZ=false;
    STATE_ERR_KZ=false;
//    STATE_CHECK_FREE_DB=false;
//    l_od.clear();
    l_otceps.clear();
    l_os.clear();

}

QList<SignalDescription> tos_Rc::acceptOutputSignals()
{
    QList<SignalDescription> l;
    rc->setSIGNAL_ERR_LS(rc->SIGNAL_ERR_LS().innerUse());l << rc->SIGNAL_ERR_LS();
    rc->setSIGNAL_ERR_LZ(rc->SIGNAL_ERR_LZ().innerUse());l << rc->SIGNAL_ERR_LZ();
    rc->setSIGNAL_ERR_KZ(rc->SIGNAL_ERR_KZ().innerUse());l << rc->SIGNAL_ERR_KZ();

    rc->setSIGNAL_BUSY_DSO(rc->SIGNAL_BUSY_DSO().innerUse());         l<<rc->SIGNAL_BUSY_DSO();
    rc->setSIGNAL_BUSY_DSO_ERR(rc->SIGNAL_BUSY_DSO_ERR().innerUse()); l<<rc->SIGNAL_BUSY_DSO_ERR();
    rc->setSIGNAL_INFO_DSO(rc->SIGNAL_INFO_DSO().innerUse());         l<<rc->SIGNAL_INFO_DSO();
    rc->setSIGNAL_BUSY_DSO_STOP(rc->SIGNAL_BUSY_DSO_STOP().innerUse());l<<rc->SIGNAL_BUSY_DSO_STOP();
    rc->setSIGNAL_BUSY_DSO_OSTOP(rc->SIGNAL_BUSY_DSO_OSTOP().innerUse());l<<rc->SIGNAL_BUSY_DSO_OSTOP();


    rc->SIGNAL_INFO_DSO().getBuffer()->setSizeData(DSO_Data_Max*sizeof(t_OsyCell_21));



    return l;
}

void tos_Rc::state2buffer()
{
    rc->SIGNAL_ERR_LS().setValue_1bit(rc->STATE_ERR_LS());
    rc->SIGNAL_ERR_LZ().setValue_1bit(rc->STATE_ERR_LZ());
    rc->SIGNAL_ERR_KZ().setValue_1bit(STATE_ERR_KZ);

    rc->SIGNAL_BUSY_DSO().setValue_1bit(rc->STATE_BUSY_DSO());
    rc->SIGNAL_BUSY_DSO_ERR().setValue_1bit(rc->STATE_BUSY_DSO_ERR());
    rc->SIGNAL_BUSY_DSO_STOP().setValue_1bit(rc->STATE_BUSY_DSO_STOP());
    rc->SIGNAL_BUSY_DSO_OSTOP().setValue_1bit(rc->STATE_BUSY_DSO_OSTOP());

    t_OsyCell_21 d;
    d.V=l_os.size();
    d.Vel=v_dso*10;
    if (v_dso==_undefV_) d.Vel=0;
    rc->SIGNAL_INFO_DSO().setValue_data(&d,sizeof (d));


}

//TOtcepData tos_Rc::od(int sf) const
//{
//    TOtcepData d;
//    if (!l_od.isEmpty()) {
//        if (sf==0) d=l_od.first();
//        if (sf==1) d=l_od.last();
//    } else {
//        if (!l_otceps.isEmpty()){
//            d.p=_pOtcepMiddle;
//            if (sf==0) d.num=l_od.first().num;
//            if (sf==1) d.num=l_od.last().num;
//        }
//    }
//    return d;
//}

//tos_OtcepData *tos_Rc::otcepOnRc(int sf)
//{
//    if (l_otceps.isEmpty()){
//        int num=0;
//        if (sf==0) num=l_otceps.first();
//        if (sf==1) num=l_otceps.last();
//        if (TOS->mNUM2OD.contains(num)){
//            return TOS->mNUM2OD[num];
//        }
//    }
//    return nullptr;
//}

//void tos_Rc::remove_od(TOtcepData o)
//{
//    l_od.removeAll(o);
//}

void tos_Rc::reset_num(int num)
{
    if (l_otceps.contains(num)) l_otceps.removeAll(num);
    for (TOtcepDataOs &os :l_os){
        if (os.num==num) {
            os.num=0;
        }
    }
}

void tos_Rc::set_v_dso(const QDateTime &T)
{

    v_dso=_undefV_;
    // находим ос с макс времнем
    if (!l_os.isEmpty()) {
        auto os0=l_os.first();
        for (const TOtcepDataOs & os : qAsConst(l_os)){
            if ((os.v!=_undefV_)&&(os.t>os0.t)) os0=os;
        }
        if (os0.v!=_undefV_){
            v_dso=os0.v;
            if (os0.d==_back) v_dso=-v_dso;
        }
    }
    // ищем стопы
    // выставляем признак остановки

    bool allstop=false;
    bool otcstop=false;
    if (!l_os.isEmpty()) {
        qreal len_rc=40;
        if (rc->LEN()!=0) len_rc=rc->LEN();
        auto os0=l_os.first();
        if (os0.d==_back){
            otcstop=true;
        } else {
            // находим крайнюю ос со значимой скростью
            for (int i=0;i<l_os.size();i++){
                os0=l_os[l_os.size()-i-1];
                if (os0.v!=_undefV_) break;
            }
            if ((os0.v!=_undefV_)&&(os0.v>0)){
                // проверяем что должен уже проехать
                auto ms=os0.t.msecsTo(T);
                qint64 ms0=len_rc/os0.v *3600.;
                // увеличим на 10%
                ms0=ms0*1.1;
                if (ms>ms0) otcstop=true;
            }
        }


        // находим ос с макс времнем
        auto t0=l_os.first().t;
        if (t0<l_os.last().t) t0=l_os.last().t;

        auto ms=t0.msecsTo(T);
        qint64 ms0=len_rc*3600.; // 1 км/ч
        if (ms>ms0) allstop=true;
    }
    if (allstop) v_dso=0;
    rc->setSTATE_BUSY_DSO_STOP(allstop);
    rc->setSTATE_BUSY_DSO_OSTOP(otcstop);


}

