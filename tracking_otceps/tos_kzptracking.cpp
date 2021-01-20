#include "tos_kzptracking.h"


#include "m_otcep.h"
#include "tos_otcepdata.h"
#include "tos_rctracking.h"
#include "trackingotcepsystem.h"

const int _bazalen=15;

tos_KzpTracking::tos_KzpTracking(TrackingOtcepSystem *parent,tos_Rc *trc) : tos_RcTracking(parent,trc)
{
    rc_park=qobject_cast<m_RC_Gor_Park*>(trc->rc);
    trc->useRcTracking=false;
    resetStates();
}

void tos_KzpTracking::resetStates()
{
    tos_RcTracking::resetStates();
    memset(&curr_state_kzp,0,sizeof(curr_state_kzp));
    memset(&prev_state_kzp,0,sizeof(prev_state_kzp));
    prev_kzp=0;curr_kzp=0;
}


void tos_KzpTracking::prepareCurState()
{
    rc_tracking_comleted=false;
    prev_state_kzp=curr_state_kzp;
    // запоминаем
    prev_kzp=curr_kzp;

    for (int j=0;j<3;j++)
        prev_rc3[j]=curr_rc3[j];

    // определяем соседей
    // RC RC[0] RC[1]
    for (int j=0;j<3;j++) curr_rc3[j]=nullptr;
    curr_rc3[0]=trc->next_rc[_back];
    if (curr_rc3[0])  curr_rc3[1]=curr_rc3[0]->next_rc[_back];
}


void tos_KzpTracking::setCurrState()
{
    curr_kzp=rc_park->STATE_KZP_D();
    // формируем строчки  состояние РЦ относительно отцепа в RC;
    //RC RC0 RC1  <<D0
    // 0   1  2
    TOtcepData od=trc->od(1);
    curr_state_kzp.rcos3[0]=kzp_rc_state();
    curr_state_kzp.rcos3[1]=busyOtcepStaotcepteN(curr_rc3[0],_pOtcepStart,od.num);
    curr_state_kzp.rcos3[2]=busyOtcepStaotcepteN(curr_rc3[1],_pOtcepStart,od.num);


    // при изменении соседей пропускаем ход
    for (int j=0;j<2;j++)
        if (curr_rc3[j]!=prev_rc3[j]) {
            prev_state_kzp=curr_state_kzp;
        }
}


tos_RcTracking::TRcBOS tos_KzpTracking::kzp_rc_state()
{
    TRcBOS b;
    if (curr_kzp<=_bazalen) b.BS=_MM;
    if (curr_kzp>_bazalen) b.BS=_LL;

    if ((prev_kzp>_bazalen*2)  && (curr_kzp<_bazalen)) b.BS=_LM;
    if ((prev_kzp<_bazalen)   && (curr_kzp>_bazalen*2)) b.BS=_ML;

    if (!trc->l_otceps.isEmpty()){
        auto od=trc->od(1);
        b.OS=otcepState(od);
    }

    return b;
}

void tos_KzpTracking::doCmd(int cmd, const QDateTime &T)
{
    tos_Rc* from_trc=curr_rc3[0];
    switch (cmd) {
    case cmdMove_1_ToFront:
        moveOd(from_trc,_1od,_forw,T,_normal_track);
        x_narast=0;
        break;
    case cmdMove_A_ToFront:
        moveOd(from_trc,_allod,_forw,T,_normal_track);
        x_narast=rc_park->STATE_KZP_D();dtNarast=T;
        break;
    case cmdMove_1_ToBack:
        moveOd(from_trc,_1od,_back,T,_normal_track);
        x_narast=0;
        break;
    case cmdReset_kzp:{
        foreach (auto num,trc->l_otceps){
            TOS->resetTracking(num);
        }
        x_narast=0;
    }
        break;
    }
}

void tos_KzpTracking::closePred(const QDateTime &T)
{
    // выставляем свойства отцепов на кзп
    for (int i=0;i<trc->l_otceps.size();i++){
        auto num=trc->l_otceps[i];
        if (TOS->mNUM2OD.contains(num)){
            auto otcep=TOS->mNUM2OD[num]->otcep;
            if (i<trc->l_otceps.size()-1){
                if (otcep->STATE_KZP_OS()==m_Otcep::kzpMoving) otcep->setSTATE_KZP_OS(m_Otcep::kzpClosed);
            } else {
                otcep->setSTATE_KZP_D(rc_park->STATE_KZP_D());
                // вычисляем остановку по окончанию нарастания
                if (otcep->STATE_KZP_OS()!=m_Otcep::kzpStopped){
                    if (x_narast<rc_park->STATE_KZP_D()){
                        x_narast=rc_park->STATE_KZP_D();
                        dtNarast=T;
                    } else {
                        if ((x_narast!=0)&&(dtNarast.msecsTo(T)>=5000))
                            otcep->setSTATE_KZP_OS(m_Otcep::kzpStopped);
                    }
                }
            }
        }
    }
}


void tos_KzpTracking::work(const QDateTime &T)
{

    static t_kzp_pairs kzp_steps[]={
        //    RC        RC0        RC1  <<D0
        // нормальное прохождение головы вперед
        { {{{_LL,_T0},{_B1,_tx},{_xx,_xx}}} ,
          {{{_LM,_T0},{_B1,_tx},{_xx,_xx}}} ,cmdMove_1_ToFront },
        { {{{_LL,_T0},{_B1,_tx},{_xx,_xx}}} ,
          {{{_LM,_T0},{_B0,_tx},{_xx,_xx}}} ,cmdMove_A_ToFront },

        { {{{_MM,_Tx},{_B1,_TF},{_xx,_xx}}} ,
          {{{_MM,_Tx},{_B0,_TF},{_xx,_xx}}} ,cmdMove_A_ToFront },
        { {{{_MM,_Tx},{_B1,_tx},{_B0,_xx}}} ,
          {{{_MM,_Tx},{_B0,_tx},{_B0,_xx}}} ,cmdMove_A_ToFront },

        { {{{_MM,_Tx},{_B1,_TF},{_B0,_xx}}} ,
          {{{_LL,_Tx},{_B1,_TF},{_B0,_xx}}} ,cmdMove_1_ToFront },


        // выезд назад
        { {{{_MM,_TF},{_B0,_T0},{_xx,_xx}}} ,
          {{{_MM,_TF},{_B1,_T0},{_xx,_xx}}} ,cmdMove_1_ToBack },
        { {{{_MM,_TS},{_B1,_Tx},{_xx,_xx}}} ,
          {{{_ML,_TS},{_B1,_Tx},{_xx,_xx}}} ,cmdMove_1_ToBack },
        { {{{_MM,_TS},{_B1,_Tx},{_xx,_xx}}} ,
          {{{_LL,_TS},{_B1,_Tx},{_xx,_xx}}} ,cmdMove_1_ToBack },

        // непонятный сброс назад
        { {{{_LL,_Tx},{_B0,_xx},{_xx,_xx}}} ,
          {{{_LM,_Tx},{_B0,_xx},{_xx,_xx}}} ,cmdReset_kzp },
        { {{{_LL,_TF},{_B1,_T0},{_xx,_xx}}} ,
          {{{_LM,_TF},{_B1,_T0},{_xx,_xx}}} ,cmdReset_kzp }

    };

    prepareCurState();
    // собираем текущую пару состояний
    setCurrState();

    int cmd=_none;
    // ищем
    int size_steps=sizeof(kzp_steps)/sizeof(kzp_steps[0]);
    for (int i=0;i<size_steps;i++){
        if (!prev_state_kzp.likeTable(kzp_steps[i].prev_state)) continue;
        if (!curr_state_kzp.likeTable(kzp_steps[i].curr_state)) continue;
        cmd=kzp_steps[i].cmd;
        break;
    }
    doCmd(cmd,T);

    closePred(T);


}

