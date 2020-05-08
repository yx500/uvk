#include "tos_kzptracking.h"


#include "m_otcep.h"
#include "tos_otcepdata.h"
#include "tos_rctracking.h"

const int _bazalen=15;

tos_KzpTracking::tos_KzpTracking(QObject *parent, m_RC_Gor_Park * rc_park) : tos_RcTracking(parent,rc_park)
{
    this->rc_park=rc_park;
    rc2[0]=rc_park->getNextRC(1,0);
    rc2[1]=rc2[0]->getNextRC(1,0);
    rc_park->rcs->useRcTracking=false;
    useRcTracking=false;
    resetStates();
}

void tos_KzpTracking::resetStates()
{

    foreach (m_Otcep *otcep, l_otceps) {
        if (otcep->STATE_KZP_OS()==m_Otcep::kzpMoving) otcep->setSTATE_KZP_OS(m_Otcep::kzpUnknow);
        rc_park->rcs->removeOtcep(otcep);
        if (otcep->STATE_LOCATION()==m_Otcep::locationOnPark) otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
    }
    tos_RcTracking::resetStates();


    memset(&curr_state_kzp,0,sizeof(curr_state_kzp));
    memset(&prev_state_kzp,0,sizeof(prev_state_kzp));
    prev_kzp=0;
}


T_kzp_rc_state tos_KzpTracking::kzp_rc_state()
{
    bool MM=false;
    bool LL=false;
    bool LM=false;
    bool ML=false;
    int curr_kzp=rc_park->STATE_KZP_D();

    if (curr_kzp<=_bazalen) MM=true;
    if (curr_kzp>_bazalen) LL=true;

    if ((prev_kzp>_bazalen*2)  && (curr_kzp<_bazalen)) LM=true;
    if ((prev_kzp<_bazalen)   && (curr_kzp>_bazalen*2)) ML=true;

    if (rc_park->rcs->l_otceps.isEmpty()){
        if (ML) return _MLT0;
        if (LM) return _LMT0;
        if (MM) return _MMT0;
        if (LL) return _LLT0;

    } else {
        m_Otcep *otcep=rc_park->rcs->l_otceps.last();
        if (otcep->RCF==rc_park) {
            if (ML) return _MLTF;
            if (LM) return _LMTF;
            if (MM) return _MMTF;
            if (LL) return _LLTF;

        }
        if (ML) return _MLTS;
        if (LM) return _LMTS;
        if (MM) return _MMTS;
        if (LL) return _LLTS;

    }

    return _NNN;
}


void tos_KzpTracking::work(const QDateTime &T)
{

    static t_kzp_pairs kzp_steps[]={
        { {s_ok , _LLT0   ,_tsB1 }  ,
          {s_ok , _LMT0   ,_tsB1 }  , cmdMoveStartToPark  },
        { {s_ok , _LLT0   ,_tsfB1},
          {s_ok , _LMT0   ,_tsfB1}, cmdMoveStartToPark  },
        { {s_ok , _LLT0   ,_tsfB1},
          {s_ok , _LMT0   ,_tsfB0}, cmdMoveStartFinishToPark  }, // маловероятно

        { {s_ok , _LLTF   ,_tsB1} ,
          {s_ok ,_LMTF    ,_tsB1} , cmdMoveStartToPark  },
        { {s_ok , _LLTF   ,_tsfB1},
          {s_ok ,_LMTF    ,_tsfB1}, cmdMoveStartToPark  },

        { {s_ok , _MMTS   ,_TFB1} ,
          {s_ok , _MMTS   ,_TFB0} , cmdMoveFinishToPark   },
        { {s_ok , _MMTF   ,_tsfB1},
          {s_ok , _MMTF   ,_tsfB0}, cmdMoveStartFinishToPark  },

        { {s_ok , _LLTF   ,_xx}   ,
          {s_ok , _LLTF   ,_xx}   , cmdUpdateLast   },
        { {s_ok , _MMTF   ,_xx}   ,
          {s_ok , _LLTF   ,_xx}   , cmdUpdateLast   },
        { {s_ok , _MMTF   ,___B0} ,
          {s_ok , _MMTF   ,___B0} , cmdUpdateLast   },
        { {s_ok , _MMTF   ,___B1} ,
          {s_ok , _MMTF   ,___B1} , cmdUpdateLast   },

        { {s_ok , _LLTF   ,___B0} ,
          {s_ok ,_LMTF    ,___B0} , cmdReset   },
        { {s_ok , _LLTF   ,___B1} ,
          {s_ok ,_LMTF    ,___B1} ,cmdReset   },

        { {s_ok , _MMTS   ,_TxB1} ,
          {s_ok , _MLTS   ,_TxB1} , cmdReset/*cmdMoveStartToRc0*/ },

        { {s_ok , _MMTF   ,_txB1} ,
          {s_ok , _MLTS   ,_txB1} , cmdReset }
    };


    // сбрасываем парк
    foreach (auto otcep,rc_park->rcs->l_otceps) {
        // голова дб  в парке!
        if (otcep->RCS!=rc_park){
            resetStates();
            return;
        }
    }


    // собираем текущее и пред состояние
    prev_state_kzp=curr_state_kzp;

    curr_state_kzp.flag=s_ok;
    curr_state_kzp.kzpos=kzp_rc_state();
    curr_state_kzp.rcos0=busyOtcepState(rc2[0],rc);

    T_kzp_command cmd=cmdNothing;

    //    bool bnew =false;
    //    if ((step.curr_kzpos!=step.pred_kzpos) || (step.curr_rcos0!=step.pred_rcos0)){
    //        bnew=true;
    //    }

    m_Otcep *lastOtcepOnPark=nullptr;
    if (!rc_park->rcs->l_otceps.isEmpty()) lastOtcepOnPark=rc_park->rcs->l_otceps.last();

    // ищем
    int size_steps=sizeof(kzp_steps)/sizeof(kzp_steps[0]);
    for (int i=0;i<size_steps;i++){
        if (kzp_steps[i].prev_state.flag!=prev_state_kzp.flag) continue;
        if (kzp_steps[i].curr_state.flag!=curr_state_kzp.flag) continue;
        if (kzp_steps[i].prev_state.kzpos!=prev_state_kzp.kzpos) continue;
        if (kzp_steps[i].curr_state.kzpos!=curr_state_kzp.kzpos) continue;
        if (!prev_state_kzp.likeTable(kzp_steps[i].prev_state)) continue;
        if (!curr_state_kzp.likeTable(kzp_steps[i].curr_state)) continue;
        cmd=kzp_steps[i].cmd;
        break;
    }
    m_Otcep *otcepOnTP=nullptr;
    if (!rc2[0]->rcs->l_otceps.isEmpty()) otcepOnTP=rc2[0]->rcs->l_otceps.last();

    switch (cmd) {
    case cmdMoveStartToPark:
        if (lastOtcepOnPark){
            if (lastOtcepOnPark->STATE_KZP_OS()!=m_Otcep::kzpStopped) lastOtcepOnPark->setSTATE_KZP_OS(m_Otcep::kzpClosed);
        }
        x_narast=0;
        if (otcepOnTP) {
            otcepOnTP->setSTATE_KZP_OS(m_Otcep::kzpUnknow);
            otcepOnTP->tos->setOtcepSF(rc_park,otcepOnTP->RCF,T);
            otcepOnTP->tos->rc_tracking_comleted[0]=true;
        }
        break;
    case cmdMoveFinishToPark:
        x_narast=rc_park->STATE_KZP_D();dtNarast=T;
        if (otcepOnTP) {
            otcepOnTP->setSTATE_LOCATION(m_Otcep::locationOnPark);
            otcepOnTP->setSTATE_KZP_OS(m_Otcep::kzpMoving);
            otcepOnTP->tos->setOtcepSF(rc_park,rc_park,T);
            otcepOnTP->tos->rc_tracking_comleted[0]=true;
            otcepOnTP->tos->rc_tracking_comleted[1]=true;
        }
        break;

    case cmdMoveStartFinishToPark:
        x_narast=rc_park->STATE_KZP_D();dtNarast=T;
        if (lastOtcepOnPark){
            if (lastOtcepOnPark->STATE_KZP_OS()!=m_Otcep::kzpStopped) lastOtcepOnPark->setSTATE_KZP_OS(m_Otcep::kzpClosed);
        }
        if (otcepOnTP){
            otcepOnTP->setSTATE_KZP_OS(m_Otcep::kzpMoving);
            otcepOnTP->setSTATE_LOCATION(m_Otcep::locationOnPark);
            otcepOnTP->tos->setOtcepSF(rc_park,rc_park,T);
            otcepOnTP->tos->rc_tracking_comleted[0]=true;
            otcepOnTP->tos->rc_tracking_comleted[1]=true;
        }
        break;
    case cmdUpdateLast:
        if (lastOtcepOnPark) {
            // вычисляем остановку по окончанию нарастания
            if (lastOtcepOnPark->STATE_KZP_OS()!=m_Otcep::kzpStopped){
                if (x_narast<rc_park->STATE_KZP_D()){
                    x_narast=rc_park->STATE_KZP_D();
                    dtNarast=T;
                } else {
                    if (dtNarast.msecsTo(T)>=5000)
                        lastOtcepOnPark->setSTATE_KZP_OS(m_Otcep::kzpStopped);
                }
            }
        }
        break;
    case cmdReset:
        resetStates();
        break;
    default:
        break;
    }

    // выставляем свойства отцепа
    if (!rc_park->rcs->l_otceps.isEmpty()){
        rc_park->rcs->l_otceps.last()->setSTATE_KZP_D(rc_park->STATE_KZP_D());
    }
    for (int i=rc_park->rcs->l_otceps.size()-2;i>=0;i--){
        m_Otcep * otcep=rc_park->rcs->l_otceps[i];
        if (otcep->STATE_KZP_OS()==m_Otcep::kzpMoving) otcep->setSTATE_KZP_OS(m_Otcep::kzpClosed);
    }

    // запоминаем
    prev_kzp=rc_park->STATE_KZP_D();

}

