#include "tos_rctracking.h"
#include "tos_otcepdata.h"

tos_RcTracking::T_BusyOtcepState tos_RcTracking::busyOtcepState(int state_busy,int otcep_num,TOtcepPart otcep_part,int otcep_main_num)
{
    if (otcep_num==0){
        if (state_busy) return ___B1;
        return ___B0;
    }
    if (otcep_num==otcep_main_num){
        if (otcep_part==_pOtcepStartEnd){
            if (state_busy) return _TSFB1; else return _TSFB0;
        }
        if (otcep_part==_pOtcepStart){
            if (state_busy) return _TSB1; else return _TSB0;
        }
        if (otcep_part==_pOtcepEnd){
            if (state_busy) return _TFB1; else return _TFB0;
        }
        if (otcep_part==_pOtcepMiddle){
            if (state_busy) return _TMB1; else return _TMB0;
        }
        if (state_busy) return _TMB1; else return _TMB0;
    } else {
        if (otcep_part==_pOtcepStartEnd){
            if (state_busy) return _tsfB1; else return _tsfB0;
        }
        if (otcep_part==_pOtcepStart){
            if (state_busy) return _tsB1; else return _tsB0;
        }
        if (otcep_part==_pOtcepEnd){
            if (state_busy) return _tfB1; else return _tfB0;
        }
        if (otcep_part==_pOtcepMiddle){
            if (state_busy) return _tmB1; else return _tmB0;
        }
        if (state_busy) return _tmB1; else return _tmB0;

    }

    return _NN;
}


tos_RcTracking::T_BusyOtcepState tos_RcTracking::busyOtcepState(const m_RC *rc,const m_RC *rcMain)
{
    if (rc==nullptr) return _NN;
    int state_busy=rc->STATE_BUSY();
    TOtcepPart otcep_part=_pOtcepPartUnknow;
    int otcep_num=0;
    if (!rc->rcs->l_otceps.isEmpty()) {
        m_Otcep *otcep=rc->rcs->l_otceps.last();
        if (otcep->STATE_LOCATION()!=m_Otcep::locationUnknow){
            otcep_num=otcep->NUM();
            if ((otcep->RCS==rc)&&((otcep->RCF==rc))) otcep_part=_pOtcepStartEnd; else
                if (otcep->RCS==rc) otcep_part=_pOtcepStart; else
                    if (otcep->RCF==rc) otcep_part=_pOtcepEnd; else
                        otcep_part=_pOtcepMiddle;
        }
    }
    int otcep_main_num=0;
    if (rcMain==nullptr){
        otcep_main_num=otcep_num;
    }  else {
        if (!rcMain->rcs->l_otceps.isEmpty()) {
            m_Otcep *otcep=rcMain->rcs->l_otceps.last();
            if (otcep->STATE_LOCATION()!=m_Otcep::locationUnknow){
                otcep_main_num=otcep->NUM();
            }
        }
    }
    return busyOtcepState(state_busy,otcep_num, otcep_part,otcep_main_num);
}


bool tos_RcTracking::busyOtcepStateLike(const T_BusyOtcepState &TS, const T_BusyOtcepState &S)
{
    if (TS==S) return true;
    if (TS==_xx) return true;
    if ((TS==_xB0) && (((S==___B0)||S==_TSB0)||(S==_TSFB0)||(S==_TFB0)||(S==_TMB0))) return true;
    if ((TS==_xB1) && (((S==___B1)||S==_TSB1)||(S==_TSFB1)||(S==_TFB1)||(S==_TMB1))) return true;
    if ((TS==_TxB0) && ((S==_TSB0)||(S==_TSFB0)||(S==_TFB0)||(S==_TMB0))) return true;
    if ((TS==_TxB1) && ((S==_TSB1)||(S==_TSFB1)||(S==_TFB1)||(S==_TMB1))) return true;
    if ((TS==_txB0) && ((S==_tsB0)||(S==_tsfB0)||(S==_tfB0)||(S==_tmB0))) return true;
    if ((TS==_txB1) && ((S==_tsB1)||(S==_tsfB1)||(S==_tfB1)||(S==_tmB1))) return true;
    return false;
}

void tos_RcTracking::addOtcep(m_Otcep *otcep)
{
    if (otcep==nullptr) return;
    if (l_otceps.contains(otcep)) return;
    if (l_otceps.size()==0) {
        l_otceps.push_back(otcep);
    } else {
        // сортируем по номеру - сначала меньшие
        for (int i=0;i<l_otceps.size();i++){
            if (otcep->NUM()<l_otceps.at(i)->NUM()){
                l_otceps.insert(i,otcep);
                return;
            }
        }
        l_otceps.push_back(otcep);
    }
}

void tos_RcTracking::removeOtcep(m_Otcep *otcep)
{
    if (otcep==nullptr){
        l_otceps.clear();
    }
    if (!l_otceps.contains(otcep)) return;
    l_otceps.removeAll(otcep);
    if (otcep->RCS==this->rc) otcep->RCS=nullptr;
    if (otcep->RCF==this->rc) otcep->RCF=nullptr;
}

void tos_RcTracking::state2buffer()
{
    rc->SIGNAL_ERR_LS().setValue_1bit(rc->STATE_ERR_LS());
    rc->SIGNAL_ERR_LZ().setValue_1bit(rc->STATE_ERR_LZ());
    rc->SIGNAL_ERR_KZ().setValue_1bit(rc->STATE_ERR_KZ());
}




tos_RcTracking::tos_RcTracking(QObject *parent, m_RC *rc) : BaseWorker(parent)
{
    this->rc=rc;
    rc->rcs=this;
    rc->addTagObject(this);
    useRcTracking=true;
    rc->setSIGNAL_ERR_LS(rc->SIGNAL_ERR_LS().innerUse());
    rc->setSIGNAL_ERR_LZ(rc->SIGNAL_ERR_LZ().innerUse());
    rc->setSIGNAL_ERR_KZ(rc->SIGNAL_ERR_KZ().innerUse());
}

void tos_RcTracking::validation(ListObjStr *l) const
{
    BaseWorker::validation(l);
}

int _find_step(tos_RcTracking::t_rc_pairs steps[],int size_steps,const tos_RcTracking::t_rc_tracking_state &prev_state,const tos_RcTracking::t_rc_tracking_state &curr_state)
{
    for (int i=0;i<size_steps;i++){
        if (!prev_state.likeTable(steps[i].prev_state)) continue;
        if (!curr_state.likeTable(steps[i].curr_state)) continue;
        return steps[i].cmd;
    }
    return tos_RcTracking::_none;
}


void tos_RcTracking::workOtcep(int sf,const QDateTime &T,int stepTable)
{
    // 1 проход - нормальное поведение
    static t_rc_pairs tos_steps_0[]={
        // нормальное прохождение головы вперед
        {{s_none ,{ _xx ,___B0 ,_TSB1 ,_xx}}  ,
         {s_none ,{ _xx ,___B1 ,_TxB1 ,_xx}},cmdMoveStartToFront },
        {{s_none , {_xx ,___B0 ,_TSFB1,_xx}}  ,
         {s_none , {_xx ,___B1 ,_TSFB1,_xx}}, cmdMoveStartToFront  },
        // нормальное прохождение хвоста вперед с проверкой дб
        {{s_none , {_xx ,_TxB1 ,_TFB1 ,_tfB1}}  ,
         {s_none , {_xx ,_TxB1 ,_TFB0 ,_tfB1}}, cmdMoveFinishToFrontCheck  },
        // нормальное прохождение хвоаста вперед
        {{s_none , {_xx ,_TxB1 ,_TxB1 ,_xx}}  ,
         {s_none , {_xx ,_TxB1 ,_TFB0 ,_xx}}, cmdMoveFinishToFront  }
    };
    // 2 проход - разбираем ситуации
    static t_rc_pairs tos_steps_1[]={
        //  не нормальное перепрыгивание
        {{s_none , {_xx ,___B0 ,_TSB0 ,_xx}}  ,
         {s_none , {_xx ,___B1 ,_TSB0 ,_xx}}, cmdMoveStartToFront  },
        {{s_none , {_xx ,___B0 ,_TSFB0,_xx}}  ,
         {s_none , {_xx ,___B1 ,_TSFB0,_xx}}, cmdMoveStartToFront  },
        //  прохождение головы назад , проверяем дребезг
        {{s_none , {_xx ,_xx ,_TSB1 ,_TxB1 }} ,
         {s_none , {_xx ,_xx ,_TSB0 ,_TxB1 }},  cmdCheckDrebezg0  },
        {{s_checking_drebezg ,{_xx ,_xx ,_TSB0 ,_TxB1 }}   ,
         {s_checking_drebezg ,{_xx ,_xx ,_TSB0 ,_TxB1 }}, cmdCheckDrebezg0  },
        {{s_checking_drebezg ,{_xx ,_xx ,_TSB0 ,_TxB1 }}   ,
         {s_none ,            {_xx ,_xx ,_TSB0 ,_TxB1 }}, cmdMoveStartToBack  },

        //  прохождение хвоста назад , проверяем дребезг
        {{s_none , {_xx ,_xx ,_TSFB1 ,___B0 }} ,
         {s_none , {_xx ,_xx ,_TSFB1 ,___B1 }},  cmdCheckDrebezg1onRC3  },
        {{s_checking_drebezg1OnRC3 ,{_xx ,_xx ,_TSFB1 ,___B1 }}   ,
         {s_checking_drebezg1OnRC3 ,{_xx ,_xx ,_TSFB1 ,___B1 }}, cmdCheckDrebezg1onRC3  },
        {{s_checking_drebezg1OnRC3 ,{_xx ,_xx ,_TSFB1 ,___B1 }}   ,
         {s_none ,                  {_xx ,_xx ,_TSFB1 ,___B1 }}, cmdMoveFinishToBack  },
        //  прохождение хвоста назад , проверяем дребезг
        {{s_none , {_xx ,_xx ,_TFB1 ,___B0 }} ,
         {s_none , {_xx ,_xx ,_TFB1 ,___B1 }},  cmdCheckDrebezg1onRC3  },
        {{s_checking_drebezg1OnRC3 ,{_xx ,_xx ,_TFB1 ,___B1 }}   ,
         {s_checking_drebezg1OnRC3 ,{_xx ,_xx ,_TFB1 ,___B1 }}, cmdCheckDrebezg1onRC3  },
        {{s_checking_drebezg1OnRC3 ,{_xx ,_xx ,_TFB1 ,___B1 }}   ,
         {s_none ,                  {_xx ,_xx ,_TFB1 ,___B1 }}, cmdMoveFinishToBack  },


        //  прохождение хвоста назад  без дребезга
//        {{s_none , {_xx ,_xx ,_TSFB1,___B0 }} ,
//         {s_none , {_xx ,_xx ,_TSFB1,___B1 }}, cmdMoveFinishToBack  },
//        {{s_none , {_xx ,_TxB1 ,_TFB1, ___B0 }} ,
//         {s_none , {_xx ,_TxB1 ,_TFB1 ,___B1 }}, cmdMoveFinishToBack  },

        //  протяжка вперед по занятости
        //    {s_none , {_xx ,___B1 ,_TSFB1,___B0 } ,
        //     s_none , {_xx ,___B1 ,_TSFB0,___B0 }, cmdMoveStartBackToRc1  },
        //    {s_none , {_xx ,___B1 ,_TSFB1,_txB1 } ,
        //     s_none , {_xx ,___B1 ,_TSFB0,_txB1 }, cmdMoveStartBackToRc1  },
        //  проталкивание впереди идущего отцепа
        {{s_none , {_xx ,_tfB1 ,_TSFB1,___B0 }} ,
         {s_none , {_xx ,_tfB1 ,_TSFB0,___B0 }}, cmdPushStartFinishToRc1  },
        //  занимаем непонятную занятость
        {{s_none , {_xx ,___B1 ,_TSFB1,___B0 }} ,
         {s_none , {_xx ,___B1 ,_TSFB0,___B0 }}, cmdMoveStartFinishToRc1  },



        //  коррекция хвоста
        {{s_none ,{_xx ,_TxB1 ,_TMB0 ,_xx   }} ,
         {s_none ,{_xx ,_TxB1 ,_TFB0 ,_xx   }}, cmdMoveFinishToFront  },
        {{s_none ,{_TxB1 ,_TMB1 ,_TFB0 ,_xx   }} ,
         {s_none ,{_TxB1 ,_TMB0 ,_TFB0 ,_xx   }}, cmdMoveFinishToRC0  },
         //  ложжная свободность
        {{s_none ,{___B0 ,___B0 ,_TSB1 ,_xx   }} ,
         {s_none ,{___B1 ,___B0 ,_TSB1 ,_xx   }}, cmdSetLSStartToRc0  },
        {{s_none ,{___B0 ,___B0 ,_TSFB1 ,_xx   }} ,
         {s_none ,{___B1 ,___B0 ,_TSFB1 ,_xx   }}, cmdSetLSStartFinishToRc0  },
        //  ложжная занятость
       {{s_none ,{___B0 ,___B1 ,_TSB1 ,_xx   }} ,
        {s_none ,{___B1 ,___B1 ,_TSB1 ,_xx   }}, cmdSetLZStartToRc0  },
       {{s_none ,{___B0 ,___B1 ,_TSFB1 ,_xx   }} ,
        {s_none ,{___B1 ,___B1 ,_TSFB1 ,_xx   }}, cmdSetLZStartFinishToRc0  },
        //  пробой стыка
       {{s_none ,{___B0 ,___B0 ,_TSB1 ,_xx   }} ,
        {s_none ,{___B1 ,___B1 ,_TSB1 ,_xx   }}, cmdSetKZStartToRc0  },
       {{s_none ,{___B0 ,___B0 ,_TSFB1 ,_xx   }} ,
        {s_none ,{___B1 ,___B1 ,_TSFB1 ,_xx   }}, cmdSetKZStartToRc0  },



         //  сбой слежения
        {{s_none , {_TxB1 ,_TxB0 ,_TFB1,_xx }} ,
         {s_none , {_TxB0 ,_TxB0 ,_TFB1,_xx }}, cmdResetRCF  },
        {{s_none , {_xx ,_TMB1 ,_TFB0,_xx }} ,
         {s_none , {_xx ,_TMB0 ,_TFB0,_xx }}, cmdResetRCF  },
         //  сброс слежения
        {{s_none , {_xx ,___B0 ,_TSFB1,___B0 }} ,
         {s_none , {_xx ,___B0 ,_TSFB0,___B0 }}, cmdReset  }

    };

    if (!useRcTracking) return;
    if (l_otceps.isEmpty()) return;  // рассматриваем только занятые отцепами
    m_Otcep *otcep;
    if (sf==0) otcep=l_otceps.first();
    if (sf==1) otcep=l_otceps.last();
    if (!otcep->STATE_ENABLED()) return;
    if (otcep->STATE_LOCATION()!=m_Otcep::locationOnSpusk) return;
    if ((sf==0)&&( otcep->RCS!=rc))return;
    if ((sf==1)&&( otcep->RCF!=rc))return;
    if (otcep->tos->rc_tracking_comleted[sf]) return;
    
    // RC[0] RC[1] RC[2] RC[3]
    //              RC

    // при изменении соседей пропускаем ход
    for (int j=0;j<4;j++){
        if (curr_rc4[j]!=prev_rc4[j]) return;
    }

    m_RC *rc_next=curr_rc4[1];
    m_RC *rc_prev=curr_rc4[3];

    // флаги временных задержек
    if (prev_state.flag==s_checking_drebezg){
        qint64 ms=rc->dtFree.msecsTo(T);
        if (ms<250) curr_state.flag=s_checking_drebezg;
    }
    if (prev_state.flag==s_checking_drebezg1OnRC3){
        qint64 ms=rc_prev->dtBusy.msecsTo(T);
        if (ms<250) curr_state.flag=s_checking_drebezg1OnRC3;
    }
    int cmd=_none;
    if (stepTable==0) cmd= _find_step(tos_steps_0,sizeof(tos_steps_0)/sizeof(tos_steps_0[0]) ,prev_state,curr_state);
    if (stepTable==1) cmd= _find_step(tos_steps_1,sizeof(tos_steps_1)/sizeof(tos_steps_1[0]) ,prev_state,curr_state);

    switch (cmd) {
    case cmdMoveStartToFront:
        otcep->tos->setOtcepSF(rc_next,otcep->RCF,T);
        otcep->setSTATE_DIRECTION(0);
        otcep->setSTATE_RCS_XOFFSET(0);
        otcep->tos->rc_tracking_comleted[sf]=true;
        break;
    case cmdMoveStartToBack:
        otcep->tos->setOtcepSF(rc_prev,otcep->RCF,T);
        otcep->setSTATE_DIRECTION(1);
        otcep->tos->rc_tracking_comleted[sf]=true;
        break;
    case cmdMoveFinishToFront:
        otcep->tos->setOtcepSF(otcep->RCS,rc_next,T);
        otcep->setSTATE_DIRECTION(0);
        otcep->tos->rc_tracking_comleted[sf]=true;
        break;
    case cmdMoveFinishToFrontCheck:
    {
        // поставить флаг доп проверки
        // так как может быть освобождение под длиннобазным
        if (rc->getNextCount(0)==2) {
            otcep->RCF->rcs->setSTATE_CHECK_FREE_DB(true);
        }
            otcep->tos->setOtcepSF(otcep->RCS,rc_next,T);
            otcep->setSTATE_DIRECTION(0);
            otcep->tos->rc_tracking_comleted[sf]=true;
    }
        break;
    case cmdMoveFinishToBack:
    {
        // убедимся что пред пред свободная
        if ((rc_prev->next_rc[1]!=nullptr) &&
                (rc_prev->next_rc[1]->STATE_BUSY()==0)) {
            otcep->tos->setOtcepSF(otcep->RCS,rc_prev,T);
            otcep->setSTATE_DIRECTION(1);
            otcep->tos->rc_tracking_comleted[sf]=true;
        }
    }
        break;
    case cmdMoveStartFinishToRc1:
        otcep->tos->setOtcepSF(rc_next,rc_next,T);
        otcep->setSTATE_DIRECTION(0);
        otcep->tos->rc_tracking_comleted[0]=true;
        otcep->tos->rc_tracking_comleted[1]=true;
        break;
    case cmdMoveFinishToRC0:{
        m_RC *rc0=curr_rc4[0];
        otcep->tos->setOtcepSF(otcep->RCS,rc0,T);
        otcep->setSTATE_DIRECTION(0);
        otcep->tos->rc_tracking_comleted[0]=true;
        otcep->tos->rc_tracking_comleted[1]=true;
    }
        break;


    case cmdPushStartFinishToRc1:
    {
        // пытаемся сдвинуть отцеп
        // именно толкаем
        if (rc_next->rcs->l_otceps.size()==1){
            if (rc_next->next_rc[0]!=nullptr) {
                m_Otcep *otcep1=rc_next->rcs->l_otceps.last();
                if (otcep1->RCS!=otcep1->RCF){
                    otcep1->tos->setOtcepSF(otcep1->RCS,rc_next->next_rc[0],T);
                    otcep1->setSTATE_DIRECTION(0);
                    otcep->tos->setOtcepSF(rc_next,rc_next,T);
                    otcep->setSTATE_DIRECTION(0);
                    otcep->setSTATE_NAGON(1);
                    otcep->tos->rc_tracking_comleted[0]=true;
                    otcep->tos->rc_tracking_comleted[1]=true;
                }
            }
        }
    }
        break;
        //            case cmdSplitBackToRc0:
        //                emit change(otcep,cmdSplitBackToRc0,otcep->RCS,curr_sd[0]->rc,T);
        //                otcep->tos->tos_step_comleted[sf]=true;
        //                break;
    case cmdCheckDrebezg0:
        curr_state.flag=s_checking_drebezg;
        break;
    case cmdCheckDrebezg1onRC3:
        curr_state.flag=s_checking_drebezg1OnRC3;
        break;
    case cmdSetLSStartToRc0:
        workLSLZ(T,otcep,0,0);
        break;
    case cmdSetLSStartFinishToRc0:
        workLSLZ(T,otcep,1,0);
        break;
    case cmdSetLZStartToRc0:
        workLSLZ(T,otcep,0,1);
        break;
    case cmdSetLZStartFinishToRc0:
        workLSLZ(T,otcep,1,1);
        break;
    case cmdSetKZStartToRc0:
        workLSLZ(T,otcep,0,2);
        break;
    case cmdResetRCF:
    {
        // ищем хвост
        m_RC* _rc=otcep->RCS;
        int z=0;
        while (_rc!=nullptr)
        {
            z++;
            if (z>100) {_rc=otcep->RCS;break;}
            if (_rc->next_rc[1]==nullptr) break;
            if (_rc->STATE_BUSY()!=1) break;
            if ((!_rc->rcs->l_otceps.isEmpty()&&(!_rc->rcs->l_otceps.contains(otcep)))) break;
        }
        otcep->tos->setOtcepSF(otcep->RCS,_rc,T);
        otcep->tos->rc_tracking_comleted[1]=true;
    }
        break;
    case cmdReset:
        otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
        otcep->tos->setOtcepSF(nullptr,nullptr,T);
        otcep->tos->rc_tracking_comleted[0]=true;
        otcep->tos->rc_tracking_comleted[1]=true;
        break;
    default:
        break;
    } // switch
}

void tos_RcTracking::workLSLZ(const QDateTime &T, m_Otcep *otcep,bool sf,int lslz)
{
    // проверяем свободность перед РЦ0
    if ((curr_rc4[0]!=nullptr)&&(curr_rc4[0]->next_rc[0]!=nullptr)&&(curr_rc4[0]->next_rc[0]->STATE_BUSY()==0)){
        if (sf){
            otcep->tos->setOtcepSF(curr_rc4[0],curr_rc4[0],T);
            otcep->tos->rc_tracking_comleted[1]=true;
        }else {
            otcep->tos->setOtcepSF(curr_rc4[0],otcep->RCF,T);
        }
        otcep->setSTATE_DIRECTION(0);
        otcep->tos->rc_tracking_comleted[0]=true;
        if (lslz==0) curr_rc4[0]->setSTATE_ERR_LS(true);
        if (lslz==1) curr_rc4[0]->setSTATE_ERR_LZ(true);
        if (lslz==2) curr_rc4[0]->setSTATE_ERR_KZ(true);
    }
}





void tos_RcTracking::work(const QDateTime &T)
{

}

void tos_RcTracking::setCurrState()
{
    for (int j=0;j<4;j++){
        prev_rc4[j]=curr_rc4[j];
    }

    prev_state=curr_state;

    // снимаем ложные занятости
    if (rc->STATE_ERR_KZ()){
        if ((rc->next_rc[0]!=nullptr)&&(rc->STATE_BUSY()!=rc->next_rc[0]->STATE_BUSY()))
            rc->setSTATE_ERR_KZ(false);
    }
    if ((rc->STATE_ERR_LS())&&(rc->STATE_BUSY()==1)) rc->setSTATE_ERR_LS(false);
    if ((rc->STATE_ERR_LZ())&&(rc->STATE_BUSY()==0)) rc->setSTATE_ERR_LZ(false);

    if (FSTATE_CHECK_FREE_DB){
        if (rc->STATE_BUSY()==1) setSTATE_CHECK_FREE_DB(false);
        if ((rc->next_rc[0]!=nullptr)&&(rc->next_rc[0]->STATE_BUSY()==0)) setSTATE_CHECK_FREE_DB(false);
        if ((rc->next_rc[1]!=nullptr)&&(rc->next_rc[1]->STATE_BUSY()==0)) setSTATE_CHECK_FREE_DB(false);
    }

    // определяем соседей
    // RC[0] RC[1] RC[2] RC[3]
    //              RC

    for (int j=0;j<4;j++) curr_rc4[j]=nullptr;
    curr_rc4[2]=rc;
    if (curr_rc4[2]) curr_rc4[1]=curr_rc4[2]->next_rc[0];
    if (curr_rc4[1]) curr_rc4[0]=curr_rc4[1]->next_rc[0];
    if (curr_rc4[2]) curr_rc4[3]=curr_rc4[2]->next_rc[1];

    curr_state.flag=s_none;
    // формируем строчки  состояние РЦ относительно отцепа в RC;
    for (int j=0;j<4;j++) {
        if (curr_rc4[j]==nullptr) {
            curr_state.rcos[j]=T_BusyOtcepState::_NN;
        } else {
               curr_state.rcos[j]=busyOtcepState(curr_rc4[j],rc);
        }
    }
    setSTATE_S0(curr_state.rcos[0]);
    setSTATE_S1(curr_state.rcos[1]);
    setSTATE_S2(curr_state.rcos[2]);
    setSTATE_S3(curr_state.rcos[3]);
    setSTATE_P0(prev_state.rcos[0]);
    setSTATE_P1(prev_state.rcos[1]);
    setSTATE_P2(prev_state.rcos[2]);
    setSTATE_P3(prev_state.rcos[3]);


}

void tos_RcTracking::resetStates()
{
    BaseWorker::resetStates();
    l_otceps.clear();
    for(int i=0;i<4;i++){
        curr_rc4[i]=nullptr;
        prev_rc4[i]=nullptr;
    }
    memset(&curr_state,0,sizeof(curr_state));
    memset(&prev_state,0,sizeof(prev_state));
    FSTATE_CHECK_FREE_DB=false;
}




