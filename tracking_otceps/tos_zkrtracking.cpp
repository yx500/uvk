#include "tos_zkrtracking.h"
#include <qdebug.h>

#include "m_otceps.h"
#include "modelgroupgorka.h"
#include "tos_otcepdata.h"

/*
 * задачи
 * Выявть Голову
 * Оторвать Хвост
 * Принять хвост обратно
 * Принять голову и сбросить
 * Считать оси для текущего
 * */


tos_ZkrTracking::tos_ZkrTracking(QObject *parent, m_RC_Gor_ZKR *rc_zkr, m_Otceps *otceps,ModelGroupGorka *modelGorka=nullptr) : tos_RcTracking(parent,rc_zkr)
{
    this->rc_zkr=rc_zkr;
    this->otceps=otceps;
    this->modelGorka=modelGorka;
    useRcTracking=false;
    rc_next=rc_zkr->getNextRC(0,0);
    if (!rc_next) qCritical() << objectName() << "Нет РЦ за ЗКР" <<endl ;
    rc_prev=rc_zkr->getNextRC(1,0);
    if (!rc_prev) qCritical() << objectName() << "Нет РЦ перед ЗКР" <<endl ;
    for (int d=0;d<2;d++)
        for (int j=0;j<2;j++){
            if (qobject_cast<m_DSO_RD_21*>(rc_zkr->dso[d][j])!=nullptr){
                dsot[d][j]=new tos_DsoTracking(this,qobject_cast<m_DSO_RD_21*>(rc_zkr->dso[d][j]));
                rc_zkr->dso[d][j]->addTagObject(dsot[d][j],28);

            } else {
                qCritical() << objectName() << "Нет dsot[][]" << d <<j<<endl ;
            }

        }
    rc_zkr->setSIGNAL_ROSPUSK(rc_zkr->SIGNAL_ROSPUSK().innerUse());
}
void tos_ZkrTracking::state2buffer()
{
    for (int d=0;d<2;d++)
        for (int j=0;j<2;j++){
            dsot[d][j]->state2buffer();
        }
   rc_zkr->SIGNAL_ROSPUSK().setValue_1bit(rc_zkr->STATE_ROSPUSK());
}

void tos_ZkrTracking::resetStates()
{
    tos_RcTracking::resetStates();
    rc_zkr->resetStates();
    for (int d=0;d<2;d++)
        for (int j=0;j<2;j++)
            dsot[d][j]->resetStates();

    baza_count=0;
    memset(&prev_state_zkr,0,sizeof(prev_state_zkr));
    FSTATE_LT_OSY_CNT=0;
    FSTATE_LT_OSY_S=0;
    FSTATE_TLG_CNT=0;
    FSTATE_TLG_SOST=0;
    FSTATE_TLG_D=0;
    FSTATE_V_DSO=_undefV_;
}




int find_step(tos_ZkrTracking::t_zkr_pairs steps[],int size_steps,int current_zkr_state,const tos_ZkrTracking::t_zkr_state &prev_state,const tos_ZkrTracking::t_zkr_state &curr_state)
{
    for (int i=0;i<size_steps;i++){
        if (steps[i].ext_state!=current_zkr_state) continue;
        if (!prev_state.likeTable(steps[i].prev_state)) continue;
        if (!curr_state.likeTable(steps[i].curr_state)) continue;
        return steps[i].cmd ;
    }
    return _none;
}



void tos_ZkrTracking::work(const QDateTime &T)
{
    static t_zkr_pairs tos_zkr_steps[]={
        // нормальное выявление
        {{{_xx ,___B1 ,_xx} ,0}  ,
         {{_xx ,___B1 ,_xx} ,1}, 0, _in  },
        {{{_xx ,___B0 ,_xx} ,0}  ,
         {{_xx ,___B1 ,_xx} ,1}, 0, _in  },
        // нормальное движение головы
        {{{___B0 ,_TSFB1 ,_xx} ,1}  ,
         {{___B1 ,_TSFB1 ,_xx} ,1}, 0, _head2front  },
        // нормальное движение хвоста
        {{{_TxB1 ,_TFB1  ,_xx} ,1}  ,
         {{_TxB1 ,_TFB0  ,_xx} ,1}, 0, _back2front  },
        {{{_TxB1 ,_TFB1  ,_xx} ,0}  ,
         {{_TxB1 ,_TFB0  ,_xx} ,0}, 0, _back2front  },
        {{{_TxB1 ,_TFB1  ,_xx} ,1}  ,
         {{_TxB1 ,_TFB0  ,_xx} ,0}, 0, _back2front  },
        // зкр занята след отцепом
        {{{_TMB1 ,_TFB1  ,_xx} ,0}  ,
         {{_TMB0 ,_TFB1  ,_xx} ,0}, 0, _back2front  },
        // выбрасываем хвост при новом занятии ртдс
        {{{_xx ,_TFB1  ,___B1} ,0}  ,
         {{_xx ,_TFB1  ,___B1} ,1}, 0, _back2front_in  },

        // не нормальное движение головы проталкиваем вниз
        {{{_tfB1 ,_TSFB1 ,_xx} ,0}  ,
         {{_tfB1 ,_TSFB0 ,_xx} ,0}, 0, _pushHeadBack2front  },


        // не нормальное движение затащили отцеп обратно
        // это отрабатывет rctracking на rcn
        //    {s_ok , {_tsfB1 ,___B1 ,_xx ,1}  ,
        //            {_tsfB0 ,___B1 ,_xx ,1}, _returnHeadBack2zkr  },

        //    // не нормальное движение затащили отцеп обратно
        //    {s_ok , {_TSB1 ,_TSFB1 ,_xx ,1}  ,
        //            {_TSB0 ,_TSFB1 ,_xx ,1}, _returnHeadBack2zkr  },

        // не нормальное движение ушли
        {{{___B0 ,_TSFB1 ,___B1} ,0}  ,
         {{___B0 ,_TSFB0 ,___B1} ,0}, 0, _resetOtcepOnZKR},



        // не может поместится
        {{{_xx ,_TSFB1 ,_xx} ,1}  ,
         {{_xx ,_TSFB1 ,_xx} ,0}, 0, _error_rtds  },
        // вообще не сработал
        {{{___B0 ,___B1 ,___B1} ,0}  ,
         {{___B1 ,___B1 ,___B1} ,0}, 0, _error_rtds  },

        {{{_TxB1 ,_TFB1 ,___B1} ,1}  ,
         {{_TxB1 ,_TFB0 ,___B1} ,1}, 0, _baza  }
    };

    // собираем текущую пару состояний
    int rtds=prev_state_zkr.rtds;
    if ((rc_zkr->rtds_1()->STATE_SRAB()==1) && (rc_zkr->rtds_2()->STATE_SRAB()==1)) rtds=1;
    if ((rc_zkr->rtds_1()->STATE_SRAB()==0) && (rc_zkr->rtds_2()->STATE_SRAB()==0)) rtds=0;
    curr_state_zkr.rcos[0]=busyOtcepState(rc_next,rc_zkr);
    curr_state_zkr.rcos[1]=busyOtcepState(rc_zkr,rc_zkr);
    curr_state_zkr.rcos[2]=busyOtcepState(rc_prev,rc_zkr);
    curr_state_zkr.rtds=rtds;


    if (rc_zkr->STATE_ROSPUSK()){
        m_Otcep *otcep=nullptr;
        if (!l_otceps.isEmpty()) otcep=l_otceps.last();
        if (otcep!=nullptr){
            // информация от ДСО

            otcep->setSTATE_V_DISO(Vdso);
            otcep->setSTATE_ZKR_OSY_CNT(osy_count[1]);
            otcep->setSTATE_ZKR_VAGON_CNT((FSTATE_TLG_CNT%2==0)? FSTATE_TLG_CNT/2: FSTATE_TLG_CNT/2+1 );
        }


        // ищем переход
        int cnt=sizeof(tos_zkr_steps)/sizeof(tos_zkr_steps[0]);
        int cmd=find_step(tos_zkr_steps,cnt,0,prev_state_zkr,curr_state_zkr);
        switch (cmd) {
        case _in:
        {
            newOtcep(T);
        }
            break;
        case _head2front:
            otcep->tos->setOtcepSF(rc_next,rc_zkr,T);
            otcep->setSTATE_DIRECTION(0);
            otcep->setSTATE_OSY_CNT(osy_count[1]);
            otcep->tos->rc_tracking_comleted[0]=true;
            break;
        case _back2front:
            otcep->tos->setOtcepSF(otcep->RCS,rc_next,T);
            otcep->setSTATE_DIRECTION(0);
            otcep->setSTATE_OSY_CNT(osy_count[1]);
            otcep->setSTATE_ZKR_VAGON_CNT((FSTATE_TLG_CNT%2==0)? FSTATE_TLG_CNT/2: FSTATE_TLG_CNT/2+1 );
            reset_dso();
            otcep->tos->rc_tracking_comleted[1]=true;
            break;
        case _back2front_in:
            otcep->tos->setOtcepSF(otcep->RCS,rc_next,T);
            otcep->setSTATE_DIRECTION(0);
            otcep->tos->rc_tracking_comleted[1]=true;
            otcep->setSTATE_OSY_CNT(osy_count[1]);
            otcep->setSTATE_ZKR_VAGON_CNT((FSTATE_TLG_CNT%2==0)? FSTATE_TLG_CNT/2: FSTATE_TLG_CNT/2+1 );
            reset_dso();
            dsot[1][0]->dso->setSTATE_OSY_COUNT(osy_count[1]-osy_count[0]);
            dsot[1][1]->dso->setSTATE_OSY_COUNT(osy_count[1]-osy_count[0]);
            newOtcep(T);
            break;

        case _pushHeadBack2front:
        {
            // пытаемся сдвинуть отцеп
            // именно толкаем
            m_Otcep *prev_otcep=rc_next->rcs->l_otceps.last();
            prev_otcep->tos->setOtcepSF(prev_otcep->RCS,rc_next->next_rc[0],T);
        }
            break;
        case _resetOtcepOnZKR:
        {
            // надо проверить что в топе рееально предыдущий
            auto *arrivingOtcep=otceps->topOtcep();
            if ((arrivingOtcep==nullptr) || (otcep->NUM()==arrivingOtcep->NUM()-1)) {
                otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);
                otcep->tos->setOtcepSF(nullptr,nullptr,T);
            } else {
                otcep->tos->resetStates();
            }
        }
            break;
        case _error_rtds:
            break;

        case _baza:
            if (otcep)
                otcep->setSTATE_ZKR_BAZA(true);
            break;
        default:
            break;
        }
    }

    work_dso(T);


    prev_state_zkr=curr_state_zkr;


}

void tos_ZkrTracking::newOtcep(const QDateTime &T)
{
    if (modelGorka->STATE_REGIM()==ModelGroupGorka::regimRospusk){
        auto *otcep=otceps->topOtcep();
        if (otcep!=nullptr){
            otcep->tos->setOtcepSF(rc_zkr,rc_zkr,T);
            otcep->setSTATE_DIRECTION(0);
            otcep->setSTATE_LOCATION(m_Otcep::locationOnSpusk);
            otcep->setSTATE_ZKR_BAZA(false);
            otcep->setSTATE_ZKR_OSY_CNT(0);
            otcep->setSTATE_ZKR_PROGRESS(true);
            otcep->setSTATE_ZKR_VAGON_CNT(0);

            otcep->tos->rc_tracking_comleted[0]=true;
            otcep->tos->rc_tracking_comleted[1]=true;
        }
    }
}



void tos_ZkrTracking::reset_dso()
{
    for (int i=0;i<2;i++)
        for (int j=0;j<2;j++)
            dsot[i][j]->dso->setSTATE_OSY_COUNT(0);
    dso_pair.resetStates();
    osy_count[0]=0;
    osy_count[1]=0;
    Vdso=_undefV_;
    VdsoTime=QDateTime();

}


void tos_ZkrTracking::work_dso(const QDateTime &T)
{
    // сброс при полном освобождении
    if (    (rc_zkr->STATE_BUSY()==0) &&
            (rc_zkr->rtds_1()->STATE_SRAB()==0)&&
            (rc_zkr->rtds_2()->STATE_SRAB()==0)){

        if (dso_pair.sost!=tos_DsoPair::tlg_0)
            dso_pair.resetStates();
        reset_dso();
        return;
    }

    for (int d=0;d<2;d++)
        for (int j=0;j<2;j++){
            dsot[d][j]->work(T);
        }

    tos_DsoTracking * wdso[2];
    // выбираем работающие ДСО
    // 0- ближний датчик
    if (dsot[0][0]->dso->STATE_ERROR()==0) wdso[0]=dsot[0][0]; else wdso[0]=dsot[0][1];
    if (dsot[1][0]->dso->STATE_ERROR()==0) wdso[1]=dsot[1][0]; else wdso[0]=dsot[1][1];

//    if ((wdso[0]->dso->STATE_OSY_COUNT()!=osy_count[0]) || (wdso[0]->dso->STATE_OSY_COUNT()!=osy_count[1])){
        osy_count[0]=wdso[0]->dso->STATE_OSY_COUNT();
        osy_count[1]=wdso[1]->dso->STATE_OSY_COUNT();

        dso_pair.updateStates(osy_count[1],osy_count[0]);

        setSTATE_LT_OSY_CNT(dso_pair.c.tlg_os);
        setSTATE_LT_OSY_S(dso_pair.c.os_start);
        setSTATE_TLG_CNT(dso_pair.tlg_cnt);
        setSTATE_TLG_SOST(dso_pair.sost);
        setSTATE_TLG_D(dso_pair.d);

        // вычисляем скорость по ДСО
        if (wdso[0]->dso->STATE_DIRECT()==0){
            tos_DsoTracking::TDSO_statistic s0=wdso[0]->getStatistic(osy_count[1]);
            tos_DsoTracking::TDSO_statistic s1=wdso[1]->getStatistic(osy_count[1]);
            if ((s0.OSY_COUNT==s1.OSY_COUNT)&&(s0.OSY_COUNT!=0)){
                qint64 ms=s0.T.msecsTo(s1.T);
                if (ms!=0){
                    qreal len=wdso[1]->dso->RC_OFFSET()-wdso[0]->dso->RC_OFFSET();
                    Vdso=1.*3600*len/ms;
                    VdsoTime=T;
                }
            }
        }

        if (wdso[1]->dso->STATE_DIRECT()==1){
            tos_DsoTracking::TDSO_statistic s0=wdso[0]->getStatistic(osy_count[0]);
            tos_DsoTracking::TDSO_statistic s1=wdso[1]->getStatistic(osy_count[0]);
            if ((s0.OSY_COUNT==s1.OSY_COUNT)&&(s0.OSY_COUNT!=0)){
                qint64 ms=s0.T.msecsTo(s1.T);
                if (ms!=0){
                    qreal len=wdso[0]->dso->RC_OFFSET()-wdso[1]->dso->RC_OFFSET();
                    Vdso=1.*3600*len/ms;
                    VdsoTime=T;
                }
            }
        }
//    }
    // тут надо плясать от текущей скорости и растоянию между датчиками
    if ((Vdso!=_undefV_)&&(VdsoTime.isValid()) && (VdsoTime.msecsTo(T)>5000)) Vdso=_undefV_;
    setSTATE_V_DSO(Vdso);
}



