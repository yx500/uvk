#include "tos_zkrtracking.h"
#include <qdebug.h>

#include "m_otceps.h"
#include "modelgroupgorka.h"
#include "tos_otcepdata.h"
#include "trackingotcepsystem.h"

/*
 * задачи
 * Выявть Голову
 * Оторвать Хвост
 * Принять хвост обратно
 * Принять голову и сбросить
 * Считать оси для текущего
 * */


tos_ZkrTracking::tos_ZkrTracking(TrackingOtcepSystem *parent, tos_Rc *rc) : tos_RcTracking(parent,rc)
{
    this->rc_zkr=qobject_cast<m_RC_Gor_ZKR *>(rc->rc);
    //useRcTracking=false;
    auto rc_next=rc_zkr->getNextRC(0,0);
    if (!rc_next) qCritical() << objectName() << "Нет РЦ за ЗКР" <<endl ;
    auto rc_prev=rc_zkr->getNextRC(1,0);
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
}
QList<SignalDescription> tos_ZkrTracking::acceptOutputSignals()
{
    rc_zkr->setSIGNAL_STATE_ERROR_RTDS(rc_zkr->SIGNAL_STATE_ERROR_RTDS().innerUse());
    rc_zkr->setSIGNAL_STATE_ERROR_NERASCEP(rc_zkr->SIGNAL_STATE_ERROR_NERASCEP().innerUse());
    rc_zkr->setSIGNAL_STATE_ERROR_OSYCOUNT(rc_zkr->SIGNAL_STATE_ERROR_OSYCOUNT().innerUse());
    QList<SignalDescription> l;
    l<< rc_zkr->SIGNAL_STATE_ERROR_RTDS() <<
        rc_zkr->SIGNAL_STATE_ERROR_NERASCEP() <<
        rc_zkr->SIGNAL_STATE_ERROR_OSYCOUNT();
    return l;
}
void tos_ZkrTracking::state2buffer()
{
    for (int d=0;d<2;d++)
        for (int j=0;j<2;j++){
            dsot[d][j]->state2buffer();
        }
    rc_zkr->SIGNAL_STATE_ERROR_RTDS().setValue_1bit(rc_zkr->STATE_ERROR_RTDS());
    rc_zkr->SIGNAL_STATE_ERROR_NERASCEP().setValue_1bit(rc_zkr->STATE_ERROR_NERASCEP());
    rc_zkr->SIGNAL_STATE_ERROR_OSYCOUNT().setValue_1bit(rc_zkr->STATE_ERROR_OSYCOUNT());
}

void tos_ZkrTracking::resetStates()
{
    tos_RcTracking::resetStates();
    rc_zkr->setSTATE_ERROR_RTDS(false);
    rc_zkr->setSTATE_ERROR_NERASCEP(false);
    rc_zkr->setSTATE_ERROR_OSYCOUNT(false);
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
    baza=false;
}


void tos_ZkrTracking::prepareCurState()
{
    rc_tracking_comleted=false;
    prev_state_zkr=curr_state_zkr;
    for (int j=0;j<3;j++)
        prev_rc3[j]=curr_rc3[j];

    // определяем соседей
    // RC[1]  RC[0] RC RC[2]
    //
    for (int j=0;j<3;j++) curr_rc3[j]=nullptr;
    curr_rc3[0]=trc->next_rc[_forw];
    //if (curr_rc3[0])  curr_rc3[1]=curr_rc3[0]->next_rc[_forw];
    curr_rc3[2]=trc->next_rc[_back];
}


void tos_ZkrTracking::setCurrState()
{
    // формируем строчки  состояние РЦ относительно отцепа в RC;
    //RC1 RC0 RC RC2 <<D0
    // --  0  1   2
    TOtcepData od=trc->od(_forw);
    curr_state_zkr.rcos3[0]=busyOtcepStaotcepteN(curr_rc3[1],_pOtcepEnd,od.num);
    curr_state_zkr.rcos3[1]=busyOtcepStaotcepteM(trc,_forw);
    curr_state_zkr.rcos3[2].OS=_T0;
    curr_state_zkr.rcos3[2].BS=busyState(curr_rc3[2]);

    int rtds=prev_state_zkr.rtds;
    if (rc_zkr->RTDS_USL_OR()){
        if ((rc_zkr->rtds_1()->STATE_SRAB()==1) || (rc_zkr->rtds_2()->STATE_SRAB()==1)) rtds=1;
    } else {
        if ((rc_zkr->rtds_1()->STATE_SRAB()==1) && (rc_zkr->rtds_2()->STATE_SRAB()==1)) rtds=1;
    }
    if ((rc_zkr->rtds_1()->STATE_SRAB()==0) && (rc_zkr->rtds_2()->STATE_SRAB()==0)) rtds=0;
    curr_state_zkr.rtds=rtds;

    //    for (int j=0;j<2;j++){
    //        if (osy_count[j]==0)curr_state_zkr.dso[j]=_D0;
    //        if (osy_count[j]>0)curr_state_zkr.dso[j]=_DF;
    //        if (osy_count[j]<0)curr_state_zkr.dso[j]=_DB;
    //    }
    //if (osy_count[0]==osy_count[1]){curr_state_zkr.dso[0]=_D0;curr_state_zkr.dso[1]=_D0;}
    if (osy_count[0]==osy_count[1])curr_state_zkr.dso=_D0;
    if (osy_count[0]<osy_count[1]) curr_state_zkr.dso=_DF;
    if (osy_count[0]>osy_count[1]) curr_state_zkr.dso=_DB;

    // при изменении соседей пропускаем ход
    if (curr_rc3[0]!=prev_rc3[0]) {
        prev_state_zkr=curr_state_zkr;
    }
}



int tos_ZkrTracking::_find_step(tos_ZkrTracking::t_zkr_pairs steps[],int size_steps,const tos_ZkrTracking::t_zkr_state &prev_state,const tos_ZkrTracking::t_zkr_state &curr_state)
{
    for (int i=0;i<size_steps;i++){
        if (!prev_state.likeTable(steps[i].prev_state)) continue;
        if (!curr_state.likeTable(steps[i].curr_state)) continue;
        return steps[i].cmd;
    }
    return _none;
}


void tos_ZkrTracking::work(const QDateTime &T)
{
    static t_zkr_pairs tos_zkr_steps0[]={
        // сброс осей при заезде
        { {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},0,_D0} ,
          {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},0,_DF} ,_resetDSO_1 },
        { {{{_xx,_xx},{_B0,_xx},{_xx,_xx}},0,_D0} ,
          {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},0,_DB} ,_resetDSO_1 }, //???
        { {{{_B0,_xx},{_B0,_xx},{_xx,_xx}},0,_D0} ,
          {{{_B0,_xx},{_B1,_xx},{_xx,_xx}},0,_DB} ,_resetDSO_1 }, //???
        // сброс осей при выявлении
        { {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},0,_D0} ,
          {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},1,_D0} ,_resetDSO_0 },
        // сброс осей при выявлении
        { {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},0,_DF} ,
          {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},1,_DF} ,_resetDSO_0IF3 },
        // сброс осей при освобождении
        { {{{_xx,_xx},{_B1,_xx},{_xx,_xx}},0,_xx} ,
          {{{_xx,_xx},{_B0,_xx},{_xx,_xx}},0,_xx} ,_resetDSO_0 }

    };
    static t_zkr_pairs tos_zkr_steps1[]={
        // нормальное выявление
        { {{{_xx,_xx},{_B1,_T0},{_xx,_xx}},0,_DF} ,
          {{{_xx,_xx},{_B1,_T0},{_xx,_xx}},1,_DF} ,_in },
        { {{{_xx,_xx},{_B1,_T0},{_B1,_xx}},0,_D0} ,
          {{{_xx,_xx},{_B1,_T0},{_B1,_xx}},1,_D0} ,_in },
        { {{{_B0,_T0},{_B1,_T0},{_xx,_xx}},0,_D0} ,
          {{{_B0,_T0},{_B1,_T0},{_xx,_xx}},1,_D0} ,_in },

        // нормальное движение головы
        { {{{_B0,_T0},{_B1,_TSF},{_xx,_xx}},_xx,_xx} ,
          {{{_B1,_T0},{_B1,_TSF},{_xx,_xx}},_xx,_xx} ,cmdMove_1_ToFront },

        // нормальное движение хвоста
        { {{{_B1,_Tx},{_B1,_TF},{_xx,_xx}},_xx,_xx} ,
          {{{_B1,_Tx},{_B0,_TF},{_xx,_xx}},_xx,_xx} ,cmdMove_A_ToFront },

        // ненормальное движение
        { {{{_B1,_T0},{_B1,_TSF},{_xx,_xx}},1,_xx} ,
          {{{_B1,_T0},{_B0,_TSF},{_xx,_xx}},1,_xx} ,cmdMove_A_ToFront_E },

        // выбрасываем хвост при новом занятии
        { {{{_xx,_xx},{_B1,_Tx},{_xx,_xx}},0,_D0} ,
          {{{_xx,_xx},{_B1,_Tx},{_xx,_xx}},0,_DF} ,cmdMove_A_ToFront },

        // выбрасываем хвост при новом занятии ртдс
        { {{{_xx,_xx},{_B1,_Tx},{_xx,_xx}},0,_DF} ,
          {{{_xx,_xx},{_B1,_Tx},{_xx,_xx}},1,_DF} ,cmdMove_A_ToFront_in },

        // не нормальное движение головы проталкиваем вниз
        //        { {{{_B1,_tx},{_B1,_Tx},{_xx,_xx}},0,_DF} ,
        //          {{{_B1,_tx},{_B1,_Tx},{_xx,_xx}},1,_DF} ,_pushHeadBack2front },
        // не нормальное движение ушли
        { {{{_B0,_T0},{_B1,_TSF},{_B1,_xx}},0,_xx} ,
          {{{_B0,_T0},{_B0,_TSF},{_B1,_xx}},0,_xx} ,_resetOtcepOnZKR },

        // не может поместится
        { {{{_xx,_xx},{_B1,_TSF},{_xx,_xx}},1,_xx} ,
          {{{_xx,_xx},{_B0,_TSF},{_xx,_xx}},0,_xx} ,_error_rtds },

        // вообще не сработал
        { {{{_B0,_xx},{_B1,_xx},{_B1,_xx}},0,_DF} ,
          {{{_B1,_xx},{_B1,_xx},{_B1,_xx}},0,_DF} ,_error_rtds },

        // база
        { {{{_B1,_Tx},{_B1,_TF},{_B1,_xx}},1,_DF} ,
          {{{_B1,_Tx},{_B0,_TF},{_B1,_xx}},1,_DF} ,_baza }

    };

    prepareCurState();
    // собираем текущую пару состояний
    setCurrState();
    work_dso(T);

    // снимем ошибку РТДС
    if (curr_state_zkr.rtds==1) rc_zkr->setSTATE_ERROR_RTDS(false);

    if (curr_state_zkr.equal(prev_state_zkr))return;

    // ищем переход
    int cmd=_none;
    cmd= _find_step(tos_zkr_steps0,sizeof(tos_zkr_steps0)/sizeof(tos_zkr_steps0[0]) ,prev_state_zkr,curr_state_zkr);
    doCmd(cmd,T);
    cmd= _find_step(tos_zkr_steps1,sizeof(tos_zkr_steps1)/sizeof(tos_zkr_steps1[0]) ,prev_state_zkr,curr_state_zkr);
    doCmd(cmd,T);


    // инфа в отцеп             // информация от ДСО

    // правильней это в TOS
    auto otcep=trc->otcepOnRc(1);
    if (otcep!=nullptr){
            otcep->otcep->setSTATE_ZKR_BAZA(baza);
            otcep->otcep->setSTATE_V_DISO(Vdso);
            otcep->otcep->setSTATE_ZKR_OSY_CNT(osy_count[1]);
            otcep->otcep->setSTATE_ZKR_VAGON_CNT((FSTATE_TLG_CNT%2==0)? FSTATE_TLG_CNT/2: FSTATE_TLG_CNT/2+1 );
            checkOsyCount(otcep->otcep);
    }

}

void tos_ZkrTracking::doCmd(int cmd, const QDateTime &T)
{
    switch (cmd) {

    case _resetDSO_1:
        reset_dso(1);
        break;
    case _resetDSO_0:
        reset_dso(0);
        break;
    case _resetDSO_0IF3:
        reset_dso(3);
        break;

    case _in:
        newOtcep(T);
        break;

    case cmdMove_1_ToFront:
        tos_RcTracking::doCmd(cmdMove_1_ToFront,T);
        break;
    case cmdMove_A_ToFront:
        tos_RcTracking::doCmd(cmdMove_A_ToFront,T);
        break;
    case cmdMove_A_ToFront_E:
        tos_RcTracking::doCmd(cmdMove_A_ToFront_E,T);
        break;
    case cmdMove_A_ToFront_in:
        tos_RcTracking::doCmd(cmdMove_A_ToFront,T);
        newOtcep(T);
        break;

    case _resetOtcepOnZKR:
    {
        tos_RcTracking::doCmd(cmd_Reset,T);

    }
        break;
    case _error_rtds:
        rc_zkr->setSTATE_ERROR_RTDS(true);
        break;

    case _baza:
        baza=true;
        break;
    default:
        break;
    }
}



void tos_ZkrTracking::newOtcep(const QDateTime &T)
{
    baza=false;
    auto *otcep=TOS->getNewOtcep(trc);
    if (otcep!=nullptr){
        otcep->resetTracking();

        TOtcepData od;
        od.num=otcep->otcep->NUM();
        od.p=_pOtcepStart;
        od.d=_forw;
        od.track=_normal_track;
        trc->l_od.push_back(od);
        od.p=_pOtcepEnd;
        trc->l_od.push_back(od);

        rc_zkr->setSTATE_ERROR_NERASCEP(false);
        rc_zkr->setSTATE_ERROR_OSYCOUNT(false);
        otcep->otcep->setSTATE_DIRECTION(0);
        otcep->otcep->setSTATE_LOCATION(m_Otcep::locationOnSpusk);
        otcep->otcep->setSTATE_ZKR_BAZA(false);
        otcep->otcep->setSTATE_ZKR_OSY_CNT(0);
        otcep->otcep->setSTATE_ZKR_PROGRESS(true);
        otcep->otcep->setSTATE_ZKR_VAGON_CNT(0);
        TOS->updateOtcepsOnRc(T);
        checkNeRascep(otcep->otcep->NUM());
    }
}

void tos_ZkrTracking::checkNeRascep(int num)
{
    bool err=false;
    if ((TOS->mNUM2OD.contains(num)) && (TOS->mNUM2OD.contains(num+1))){
        // если оси  подозрительно равны своему + следующему
        auto *otcep1=TOS->mNUM2OD[num];
        auto *otcep2=TOS->mNUM2OD[num+1];
        if (otcep1->otcep->STATE_SL_OSY_CNT()+otcep2->otcep->STATE_SL_OSY_CNT()>=8){
            if (otcep1->otcep->STATE_ZKR_OSY_CNT()==
                    otcep1->otcep->STATE_SL_OSY_CNT()+otcep2->otcep->STATE_SL_OSY_CNT()
                    ) err=true;
        }

        rc_zkr->setSTATE_ERROR_NERASCEP(err);
    }
}

void tos_ZkrTracking::checkOsyCount(m_Otcep *otcep)
{
    // проверяем что оси не ушли за заданные
    if ((otcep->STATE_SL_OSY_CNT()>=4)&&
            (otcep->STATE_ZKR_OSY_CNT()>=otcep->STATE_SL_OSY_CNT()+4))
        rc_zkr->setSTATE_ERROR_OSYCOUNT(true); else
        rc_zkr->setSTATE_ERROR_OSYCOUNT(false);
}

void tos_ZkrTracking::resetOtcep2Nadvig()
{
    auto otcep=trc->otcepOnRc(1);
    if (otcep!=nullptr){
        TOS->resetTracking(otcep->otcep->NUM());
        otcep->otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);
    }
}



void tos_ZkrTracking::reset_dso(int n)
{
    if (n==0){
        for (int i=0;i<2;i++)
            for (int j=0;j<2;j++)
                dsot[i][j]->dso->setSTATE_OSY_COUNT(0);
        dso_pair.resetStates();
        osy_count[0]=0;
        osy_count[1]=0;
        Vdso=_undefV_;
        VdsoTime=QDateTime();
    }
    if (n==1){
        for (int i=0;i<2;i++)
            for (int j=0;j<2;j++)
                dsot[i][j]->dso->setSTATE_OSY_COUNT(0);
        dso_pair.resetStates();
        for (int j=0;j<2;j++)
            dsot[1][j]->dso->setSTATE_OSY_COUNT(1);
        osy_count[0]=0;
        osy_count[1]=1;
        Vdso=_undefV_;
        VdsoTime=QDateTime();

    }
    if (n==3){
        if (osy_count[1]>2){
            for (int i=0;i<2;i++)
                for (int j=0;j<2;j++)
                    dsot[i][j]->dso->setSTATE_OSY_COUNT(0);
            dso_pair.resetStates();
            osy_count[0]=0;
            osy_count[0]=1;
            Vdso=_undefV_;
            VdsoTime=QDateTime();
        }

    }

}


void tos_ZkrTracking::work_dso(const QDateTime &T)
{
    for (int d=0;d<2;d++)
        for (int j=0;j<2;j++){
            dsot[d][j]->work(T);
        }

    tos_DsoTracking * wdso[2];
    // выбираем работающие ДСО
    // 0- ближний датчик
    if (dsot[0][0]->dso->STATE_ERROR()==0) wdso[0]=dsot[0][0]; else wdso[0]=dsot[0][1];
    if (dsot[1][0]->dso->STATE_ERROR()==0) wdso[1]=dsot[1][0]; else wdso[0]=dsot[1][1];

    if ((wdso[0]->dso->STATE_OSY_COUNT()!=osy_count[0]) ||
            (wdso[1]->dso->STATE_OSY_COUNT()!=osy_count[1])){
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
    }
    //    }
    // тут надо плясать от текущей скорости и растоянию между датчиками
    if ((Vdso!=_undefV_)&&(VdsoTime.isValid()) && (VdsoTime.msecsTo(T)>5000)) Vdso=_undefV_;
    setSTATE_V_DSO(Vdso);
}



