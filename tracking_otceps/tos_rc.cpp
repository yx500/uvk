#include "tos_rc.h"
#include "tos_system.h"

tos_Rc::tos_Rc(tos_System *TOS, m_RC *rc) : BaseWorker(TOS)
{

    this->TOS=TOS;
    this->rc=rc;
    STATE_BUSY=MVP_Enums::busy_unknow;
    useRcTracking=false;
}

void tos_Rc::work(const QDateTime &T)
{

    for (int d=0;d<2;d++)
        next_rc[d]=TOS->mRc2TRC[rc->next_rc[d]];

    auto new_state=rc->STATE_BUSY();
    // дребезг 11111110 10101 000000000
    if (new_state!=STATE_BUSY){
        if ((!time_STATE_BUSY.isValid()) ||
         (time_STATE_BUSY.msecsTo(T)>500)){
            STATE_BUSY=new_state;
            time_STATE_BUSY=T;
        }
    }
    // снимаем ложные занятости
    if (STATE_ERR_KZ){
        if ((rc->next_rc[0]!=nullptr)&&(rc->STATE_BUSY()!=rc->next_rc[0]->STATE_BUSY()))
            STATE_ERR_KZ=false;
    }
    if ((STATE_ERR_LS)&&(rc->STATE_BUSY()==1)) STATE_ERR_LS=false;
    if ((STATE_ERR_LZ)&&(rc->STATE_BUSY()==0)) STATE_ERR_LZ=false;

    if (STATE_CHECK_FREE_DB){
        if (rc->STATE_BUSY()==1) STATE_CHECK_FREE_DB=false;
        if ((rc->next_rc[0]!=nullptr)&&(rc->next_rc[0]->STATE_BUSY()==0)) STATE_CHECK_FREE_DB=false;
        if ((rc->next_rc[1]!=nullptr)&&(rc->next_rc[1]->STATE_BUSY()==0)) STATE_CHECK_FREE_DB=false;
    }

}

void tos_Rc::resetStates()
{
    BaseWorker::resetStates();
    STATE_BUSY=MVP_Enums::busy_unknow;
    time_STATE_BUSY=QDateTime();
    STATE_ERR_LS=false;
    STATE_ERR_LZ=false;
    STATE_ERR_KZ=false;
    STATE_CHECK_FREE_DB=false;
    l_od.clear();
    l_otceps.clear();

}

QList<SignalDescription> tos_Rc::acceptOutputSignals()
{
    rc->setSIGNAL_ERR_LS(rc->SIGNAL_ERR_LS().innerUse());
    rc->setSIGNAL_ERR_LZ(rc->SIGNAL_ERR_LZ().innerUse());
    rc->setSIGNAL_ERR_KZ(rc->SIGNAL_ERR_KZ().innerUse());
    rc->setSIGNAL_CHECK_FREE_DB(rc->SIGNAL_CHECK_FREE_DB().innerUse());
    QList<SignalDescription> l;
    l << rc->SIGNAL_ERR_LS()<<rc->SIGNAL_ERR_LZ()<<rc->SIGNAL_ERR_KZ()<<rc->SIGNAL_CHECK_FREE_DB();
    return l;
}

void tos_Rc::state2buffer()
{
    rc->SIGNAL_ERR_LS().setValue_1bit(STATE_ERR_LS);
    rc->SIGNAL_ERR_LZ().setValue_1bit(STATE_ERR_LZ);
    rc->SIGNAL_ERR_KZ().setValue_1bit(STATE_ERR_KZ);
    rc->SIGNAL_CHECK_FREE_DB().setValue_1bit(STATE_CHECK_FREE_DB);

}

TOtcepData tos_Rc::od(int sf) const
{
    TOtcepData d;
    if (!l_od.isEmpty()) {
        if (sf==0) d=l_od.first();
        if (sf==1) d=l_od.last();
    } else {
        if (!l_otceps.isEmpty()){
            d.p=_pOtcepMiddle;
            if (sf==0) d.num=l_od.first().num;
            if (sf==1) d.num=l_od.last().num;
        }
    }
    return d;
}

tos_OtcepData *tos_Rc::otcepOnRc(int sf)
{
    if (l_otceps.isEmpty()){
        int num=0;
        if (sf==0) num=l_otceps.first();
        if (sf==1) num=l_otceps.last();
        if (TOS->mNUM2OD.contains(num)){
            return TOS->mNUM2OD[num];
        }
    }
    return nullptr;
}

void tos_Rc::remove_od(TOtcepData o)
{
    l_od.removeAll(o);
}
