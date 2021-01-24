#include "tos_rctracking.h"
#include "tos_otcepdata.h"
#include "tos_system_rc.h"



tos_RcTracking::tos_RcTracking(tos_System_RC *parent, tos_Rc *rc) : BaseWorker(parent)
{

    TOS=parent;
    this->trc=rc;
    rc->addTagObject(this);
    trc->useRcTracking=true;

}

void tos_RcTracking::validation(ListObjStr *l) const
{
    BaseWorker::validation(l);
}



int tos_RcTracking::otcepState(const TOtcepData &od)
{
    if (od.num!=0){
        if (od.p==_pOtcepStart) return _TS;
        if (od.p==_pOtcepEnd) return _TF;
        if (od.p==_pOtcepMiddle) return _TM;
    }

    return _T0;
}

int tos_RcTracking::busyState(const tos_Rc *trc)
{
    if (trc==nullptr) return _NN;
    int s=trc->STATE_BUSY;
    //if (trc->STATE_BUSY==1) s=1;
    if (trc->STATE_ERR_LS) s=_LS;
    if (trc->STATE_ERR_LZ) s=_LZ;
    if (trc->STATE_ERR_KZ) s=_KZ0+s;
    return s;
}

tos_RcTracking::TRcBOS tos_RcTracking::busyOtcepStaotcepteM(const tos_Rc *trc,int sf)
{
    // рассматриваем 2 c краю
    if (trc==nullptr) return tos_RcTracking::TRcBOS(_NN,_NN);
    tos_RcTracking::TRcBOS b;
    b.BS=busyState(trc);
    TOtcepData od1;
    TOtcepData od2;
    if (!trc->l_od.isEmpty()) {
        if (sf==0) od1=trc->l_od.first();
        if (sf==1) od2=trc->l_od.last();
    }
    if (trc->l_od.size()>1) {
        if (sf==0) od1=trc->l_od[1];
        if (sf==1) od2=trc->l_od[trc->l_od.size()-1];
    }
    if (sf==0) b.OS=otcepState(od1);
    if (sf==1) b.OS=otcepState(od2);
    if ((od1.num!=0) && (od1.num==od2.num)){
        if ((od1.p==_pOtcepStart) && (od2.p==_pOtcepEnd)) b.OS=_TSF;
    }
    return b;

}

tos_RcTracking::TRcBOS tos_RcTracking::busyOtcepStaotcepteN(const tos_Rc *trc, int sf, int main_num)
{
    // рассматриваем только края
    if (trc==nullptr) return tos_RcTracking::TRcBOS(_NN,_NN);
    if (!trc->useRcTracking) return tos_RcTracking::TRcBOS(_NN,_NN);
    tos_RcTracking::TRcBOS b;
    b.BS=busyState(trc);
    TOtcepData od=trc->od(sf);
    b.OS=otcepState(od);
    if ((od.num!=0)&& (od.num!=main_num)) b.OS=_tx;
    return b;


}

void tos_RcTracking::prepareCurState()
{
    rc_tracking_comleted=false;
    for (int d=0;d<2;d++){
        prev_state[d]=curr_state[d];

    }
    for (int j=0;j<3;j++)
        prev_rc3[j]=curr_rc3[j];

    // определяем соседей
    // RC[1] RC[0] RC RC[2]
    //
    for (int j=0;j<3;j++) curr_rc3[j]=nullptr;
    curr_rc3[0]=trc->next_rc[_forw];
    if (curr_rc3[0])  curr_rc3[1]=curr_rc3[0]->next_rc[_forw];
    curr_rc3[2]=trc->next_rc[_back];
}


void tos_RcTracking::setCurrState()
{

    // формируем строчки  состояние РЦ относительно отцепа в RC;
    //RC1 RC0 RC RC2 <<D0
    // 0   1  2   3
    for (int d=0;d<2;d++){
        TOtcepData od=trc->od(d);
        curr_state[d].rcos4[0]=busyOtcepStaotcepteN(curr_rc3[1],_pOtcepEnd,od.num);
        curr_state[d].rcos4[1]=busyOtcepStaotcepteN(curr_rc3[0],_pOtcepEnd,od.num);
        curr_state[d].rcos4[2]=busyOtcepStaotcepteM(trc,d);
        curr_state[d].rcos4[3]=busyOtcepStaotcepteN(curr_rc3[2],_pOtcepStart,od.num);
    }

    // при изменении соседей пропускаем ход
    for (int j=0;j<3;j++)
        if (curr_rc3[j]!=prev_rc3[j]) {
            prev_state[0]=curr_state[0];
            prev_state[1]=curr_state[1];
        }
}





int _find_step(tos_RcTracking::t_rc_pairs steps[],int size_steps,const tos_RcTracking::t_rc_tracking_state &prev_state,const tos_RcTracking::t_rc_tracking_state &curr_state)
{
    for (int i=0;i<size_steps;i++){
        if (!prev_state.likeTable(steps[i].prev_state)) continue;
        if (!curr_state.likeTablePrev(steps[i].curr_state,steps[i].prev_state)) continue;
        return steps[i].cmd;
    }
    return tos_RcTracking::_none;
}



void tos_RcTracking::workOtcep(int dd,const QDateTime &T,int stepTable)
{


    // 1 проход - нормальное поведение <==RC
    static t_rc_pairs tos_steps_0[]={

        // нормальное прохождение головы вперед
        { {{{_xx,_xx},{_B0,_T0},{_B1,_TSx},{_xx,_xx}}} ,
          {{{___,___},{_B1,___},{___,___},{_xx,_xx}}} ,cmdMove_1_ToFront },

        // нормальное прохождение хвоста вперед
        { {{{_xx,_xx},{_B1,_TS},{_B1,_TF},{_xx,_xx}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToFront },
        { {{{_xx,_xx},{_B1,_TM},{_B1,_TF},{_xx,_xx}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToFront }

    };
    // 2 проход - смотрим обратное движение  RC==>
    static t_rc_pairs tos_steps_1[]={

        //  прохождение хвоста назад ,
        // - на 99% это локомотив
        { {{{_xx,_xx},{_B0,_Tx},{_B1,_TF},{_B0,_T0}}} ,
          {{{___,___},{___,___},{___,___},{_B1,___}}} ,cmdMove_1_ToBack },
        { {{{_xx,_xx},{_xx,_xx},{_B1,_TSF},{_B0,_T0}}} ,
          {{{___,___},{___,___},{___,___},{_B1,___}}} ,cmdMove_1_ToBack },



        //  прохождение головы назад
        { {{{_xx,_xx},{_xx,_xx},{_B1,_TS},{_B1,_TF}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToBack },
        { {{{_xx,_xx},{_xx,_xx},{_B1,_TS},{_B1,_TM}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToBack }

    };
    // 3 проход - разбираем ситуации <==RC
    static t_rc_pairs tos_steps_2[]={

        //  не нормальное перепрыгивание
        { {{{_B0,_T0},{_B0,_T0},{_B1,_TS},{_B0,_T0}}} ,
          {{{___,___},{_B1,___},{_B0,___},{___,___}}} ,cmdMove_A_ToFront_E },


        //  проталкивание впереди идущего отцепа
        { {{{_xx,_xx},{_B1,_tx},{_B1,_TSF},{_B0,_T0}}} ,
          {{{___,___},{___,___},{_B0,___},{___,___}}} ,cmdMove_A_ToFront_Nagon },

        //  занимаем непонятную занятость

        { {{{_xx,_xx},{_B1,_T0},{_B1,_TSF},{_B0,_T0}}} ,
          {{{___,___},{___,___},{_B0,___},{___,___}}} ,cmdMove_A_ToFront_E },


        //  ложжная свободность
        { {{{_B0,_T0},{_B0,_T0},{_B1,_TSx},{_xx,_xx}}} ,
          {{{_B1,___},{___,___},{___,___},{_xx,_xx}}} ,cmdMove_1_ToFront2_LS },

        { {{{_B0,_T0},{_LS,_T0},{_B1,_TSx},{_xx,_xx}}} ,
          {{{_B1,___},{___,___},{___,___},{_xx,_xx}}} ,cmdMove_1_ToFront2 },
        { {{{_B1,_Tx},{_LS,_TM},{_B1,_TF},{_xx,_xx}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToFront2 },
        { {{{_B1,_TM},{_LS,_TM},{_B1,_TF},{_xx,_xx}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToFront2 },
        { {{{_B0,_T0},{_LS,_T0},{_B1,_TSF},{_B0,_T0}}} ,
          {{{___,___},{___,___},{_B0,___},{___,___}}} ,cmdMove_A_ToFront_E },


        //  ложжная занятость
        { {{{_B0,_T0},{_B1,_T0},{_B1,_TSx},{_xx,_xx}}} ,
          {{{_B1,___},{___,___},{___,___},{_xx,_xx}}} ,cmdMove_1_ToFront2_LZ },
        { {{{_B0,_T0},{_LZ,_TM},{_B1,_TSx},{_xx,_xx}}} ,
          {{{_B1,___},{___,___},{___,___},{_xx,_xx}}} ,cmdMove_1_ToFront2 },
        { {{{_B1,_Tx},{_LZ,_TM},{_B1,_TF},{_xx,_xx}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToFront2 },
        { {{{_B1,_T0},{_LZ,_T0},{_B1,_TSF},{_B0,_T0}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToFront_E },

        //  пробой стыка
        { {{{_B0,_T0},{_B0,_T0},{_B1,_TSx},{_xx,_xx}}} ,
          {{{_B1,___},{_B1,___},{___,___},{_xx,_xx}}} ,cmdMove_1_ToFront2_KZ },

        { {{{_B0,_T0},{_KZ0,_T0},{_B1,_TSx},{_xx,_xx}}} ,
          {{{_B1,___},{_KZ1,___},{___,___},{_xx,_xx}}} ,cmdMove_1_ToFront2 },


    };

    // 4 проход - разбираем ситуации RC==>
    static t_rc_pairs tos_steps_3[]={

        //  прохождение головы назад спорно
        { {{{_xx,_xx},{_B0,_T0},{_B1,_TSF},{_B1,_T0}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmdMove_A_ToBack_E },

        // сброс слежения
        { {{{_xx,_xx},{_xx,_xx},{_B1,_TSF},{_xx,_xx}}} ,
          {{{___,___},{___,___},{_B0,___},{_xx,_xx}}} ,cmd_Reset },



    };



    setCurrState();
    if (trc->l_od.isEmpty()) return;  // рассматриваем только занятые отцепами
    // смотрим только изменения
    if ((dd==0)&&(curr_state[0].equal(prev_state[0])))return;
    if ((dd==1)&&(curr_state[1].equal(prev_state[1])))return;
    // 1 изменение за цикл
    if (rc_tracking_comleted) return;

    int cmd=_none;
    if (stepTable==0) cmd= _find_step(tos_steps_0,sizeof(tos_steps_0)/sizeof(tos_steps_0[0]) ,prev_state[dd],curr_state[dd]);
    if (stepTable==1) cmd= _find_step(tos_steps_1,sizeof(tos_steps_1)/sizeof(tos_steps_1[0]) ,prev_state[dd],curr_state[dd]);
    if (stepTable==2) cmd= _find_step(tos_steps_2,sizeof(tos_steps_2)/sizeof(tos_steps_2[0]) ,prev_state[dd],curr_state[dd]);
    if (stepTable==3) cmd= _find_step(tos_steps_3,sizeof(tos_steps_3)/sizeof(tos_steps_3[0]) ,prev_state[dd],curr_state[dd]);

    doCmd(cmd,T);

    if (cmd!=_none) rc_tracking_comleted=true;

}

void tos_RcTracking::doCmd(int cmd, const QDateTime &T)
{
    tos_Rc* next_trc=curr_rc3[0];
    tos_Rc* next_trc2=curr_rc3[1];
    tos_Rc* prev_trc=curr_rc3[2];

    switch (cmd) {
    case cmdMove_1_ToFront:
        moveOd(next_trc,_1od,_forw,T,_normal_track);
        break;
    case cmdMove_A_ToFront:
        moveOd(next_trc,_allod,_forw,T,_normal_track);
        break;
    case cmdMove_1_ToBack:
        moveOd(prev_trc,_1od,_back,T,_normal_track);
        break;
    case cmdMove_A_ToBack:
        moveOd(prev_trc,_allod,_back,T,_normal_track);
        break;
    case cmdMove_A_ToFront_E:
        moveOd(next_trc,_allod,_forw,T,_broken_track);
        setOtcepErrorTrack(trc->od(_forw).num);
        break;
    case cmdMove_A_ToBack_E:
        moveOd(prev_trc,_allod,_back,T,_broken_track);
        setOtcepErrorTrack(trc->od(_forw).num);
        break;
    case cmdMove_A_ToFront_Nagon:
        moveOd(next_trc,_allod,_forw,T,_broken_track);
        setOtcepNagon(trc->od(_forw).num);
        break;
    case cmdMove_1_ToFront2:
        moveOd(next_trc2,_1od,_forw,T,_broken_track);
        setOtcepErrorTrack(trc->od(_forw).num);
        break;
    case cmdMove_A_ToFront2:
        moveOd(next_trc2,_allod,_forw,T,_broken_track);
        setOtcepErrorTrack(trc->od(_forw).num);
        break;

        //    case cmdPushStartFinishToRc1:
        //    {
        //        // пытаемся сдвинуть отцеп
        //        // именно толкаем
        //        bool pushok=false;
        //        if (rc_next->rcs->l_otceps.size()==1){
        //            if (rc_next->next_rc[0]!=nullptr) {
        //                m_Otcep *otcep1=rc_next->rcs->l_otceps.last();
        //                if (otcep1->RCS!=otcep1->RCF){
        //                    otcep1->tos->setOtcepSF(otcep1->RCS,rc_next->next_rc[0],T);
        //                    otcep1->setSTATE_DIRECTION(0);
        //                    otcep->tos->setOtcepSF(rc_next,rc_next,T);
        //                    otcep->setSTATE_DIRECTION(0);
        //                    otcep->setSTATE_NAGON(1);
        //                    otcep->tos->rc_tracking_comleted[0]=true;
        //                    otcep->tos->rc_tracking_comleted[1]=true;
        //                    pushok=true;
        //                }
        //            }
        //        }
        //        if (!pushok){
        //            otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
        //            otcep->tos->setOtcepSF(nullptr,nullptr,T);
        //            otcep->tos->rc_tracking_comleted[0]=true;
        //            otcep->tos->rc_tracking_comleted[1]=true;
        //        }
        //    }
        break;
        //            case cmdSplitBackToRc0:
        //                emit change(otcep,cmdSplitBackToRc0,otcep->RCS,curr_sd[0]->rc,T);
        //                otcep->tos->tos_step_comleted[sf]=true;
        //                break;

    case cmdMove_1_ToFront2_LS:
        workLSLZ(T,0);
        break;
    case cmdMove_1_ToFront2_LZ:
        workLSLZ(T,1);
        break;
    case cmdMove_1_ToFront2_KZ:
        workLSLZ(T,2);
        break;
//    case cmdResetRCF:
//    {
//        // ищем хвост
//        m_RC* _rc=otcep->RCS;
//        int z=0;
//        while (_rc!=nullptr)
//        {
//            z++;
//            m_RC* _rc_prev=_rc->next_rc[1];
//            if (z>100) {_rc=otcep->RCS;break;}
//            if (_rc_prev==nullptr) break;
//            if (_rc_prev->STATE_BUSY()!=1) break;
//            if ((!_rc_prev->rcs->l_otceps.isEmpty()&&(!_rc_prev->rcs->l_otceps.contains(otcep)))) break;
//            _rc=_rc_prev;
//        }
//        otcep->tos->setOtcepSF(otcep->RCS,_rc,T);
//        otcep->tos->rc_tracking_comleted[1]=true;
//    }
//        break;
    case cmd_Reset:
    {
        foreach (auto od,trc->l_od){
            setOtcepErrorTrack(od.num);
        }
        foreach (auto od,trc->l_od){
            TOS->resetTracking(od.num);
        }
    }
        break;
    default:
        break;
    } // switch
}


void tos_RcTracking::workLSLZ(const QDateTime &T,int lslz)
{
    // проверяем свободность перед РЦ0
    tos_Rc *next_trc2=curr_rc3[1];
    if ((next_trc2!=nullptr)&&(next_trc2->next_rc[_forw]!=nullptr)&&(next_trc2->next_rc[_forw]->STATE_BUSY==0)){
        moveOd(next_trc2,_1od,_forw,T,_broken_track);
        if (lslz==0) curr_rc3[0]->STATE_ERR_LS=true;
        if (lslz==1) curr_rc3[0]->STATE_ERR_LZ=true;
        if (lslz==2) {curr_rc3[0]->STATE_ERR_KZ=true;}
    }
}

TOtcepData _moveOd(int d, tos_Rc *rcFrom, tos_Rc *rcTo, bool bnorm)
{
    TOtcepData od;
    if (!rcFrom->l_od.isEmpty()){
        if (d==0) {
            // N <== 0
            od=rcFrom->l_od.first();
            od.track=bnorm;
            od.d=d;
            if (rcTo!=nullptr) {
                rcTo->l_od.push_back(od);

            }
            rcFrom->l_od.pop_front();
        }
        if (d==1) {
            // N ==> 0
            od=rcFrom->l_od.last();
            od.track=bnorm;
            od.d=d;
            if (rcTo!=nullptr) {
                rcTo->l_od.push_front(od);
            }
            rcFrom->l_od.pop_back();
        }
    }
    return od;
}
void tos_RcTracking::moveOd(tos_Rc*next_trc, int _1a, int d, const QDateTime &T, bool bnorm)
{
    if (!trc->l_od.isEmpty()){
        int cnt=1;
        if (_1a==_allod) cnt=trc->l_od.size();
        if (cnt>1) bnorm=_broken_track;
        // N ==> 0
        while (cnt>0){
            _moveOd(d, trc, next_trc,bnorm);
            cnt--;
        }
    }
    TOS->updateOtcepsOnRc(T);

}






void tos_RcTracking::work(const QDateTime &)
{

}



void tos_RcTracking::resetStates()
{
    BaseWorker::resetStates();
    for(int i=0;i<3;i++){
        curr_rc3[i]=nullptr;
        prev_rc3[i]=nullptr;
    }
    memset(&curr_state,0,sizeof(curr_state));
    memset(&prev_state,0,sizeof(prev_state));
}


void tos_RcTracking::setOtcepNagon(int num)
{
    if (TOS->mNUM2OD.contains(num)){
        TOS->mNUM2OD[num]->otcep->setSTATE_NAGON(true);
    }
}

void tos_RcTracking::setOtcepErrorTrack(int num)
{
    if (TOS->mNUM2OD.contains(num)){
        TOS->mNUM2OD[num]->otcep->setSTATE_ERROR_TRACK(true);
    }
}




