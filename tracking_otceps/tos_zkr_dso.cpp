#include "tos_zkr_dso.h"
#include <qdebug.h>

#include "tos_system_dso.h"
#include "tos_otcepdata.h"



tos_Zkr_DSO::tos_Zkr_DSO(tos_System_DSO *parent, tos_Rc *rc) : BaseWorker(parent)
{
    this->trc=rc;
    this->rc_zkr=qobject_cast<m_RC_Gor_ZKR *>(rc->rc);
    this->TOS=parent;

    // 0  1    <<< D0
    // 0  1
    // [0][0] [0][1] - датчики опр телег, причем [0][1] - dso_tracking
    // [1][0] [1][1] - датчики определенияя заезда
    //


    for (int i=0;i<2;i++){
        for (int j=0;j<2;j++){
            if (TOS->mDSO2TDSO.contains(rc_zkr->dso[i][j])){
                tdso[i][j]=TOS->mDSO2TDSO[rc_zkr->dso[i][j]];
            } else {
                qCritical() << objectName() << "Нет dsot[][]" << i <<j<<endl ;
            }
        }
    }
    for (int j=0;j<2;j++){
        if (TOS->mDSO2TRDSO.contains(tdso[1][j]->dso)) {
            TOS->mDSO2TRDSO[tdso[1][j]->dso]->setSTATE_ENABLED(false);
        }
    }

    dsp_pair=new tos_DsoPair(parent,tdso[0][0],tdso[0][1]);
    curr_state_zkr.sost=_otcep_unknow;

}

QList<SignalDescription> tos_Zkr_DSO::acceptOutputSignals()
{
    rc_zkr->setSIGNAL_STATE_ERROR_RTDS(rc_zkr->SIGNAL_STATE_ERROR_RTDS().innerUse());
    rc_zkr->setSIGNAL_STATE_ERROR_NERASCEP(rc_zkr->SIGNAL_STATE_ERROR_NERASCEP().innerUse());
    rc_zkr->setSIGNAL_STATE_ERROR_OSYCOUNT(rc_zkr->SIGNAL_STATE_ERROR_OSYCOUNT().innerUse());
    rc_zkr->setSIGNAL_STATE_OTCEP_UNKNOW(rc_zkr->SIGNAL_STATE_OTCEP_UNKNOW().innerUse());
    rc_zkr->setSIGNAL_STATE_OTCEP_FREE(rc_zkr->SIGNAL_STATE_OTCEP_FREE().innerUse());
    rc_zkr->setSIGNAL_STATE_OTCEP_IN(rc_zkr->SIGNAL_STATE_OTCEP_IN().innerUse());
    QList<SignalDescription> l;
    l<< rc_zkr->SIGNAL_STATE_ERROR_RTDS() <<
        rc_zkr->SIGNAL_STATE_ERROR_NERASCEP() <<
        rc_zkr->SIGNAL_STATE_ERROR_OSYCOUNT()<<
        rc_zkr->SIGNAL_STATE_OTCEP_UNKNOW()<<
        rc_zkr->SIGNAL_STATE_OTCEP_FREE()<<
        rc_zkr->SIGNAL_STATE_OTCEP_IN();
    return l;
}
void tos_Zkr_DSO::state2buffer()
{
    rc_zkr->SIGNAL_STATE_ERROR_RTDS().setValue_1bit(rc_zkr->STATE_ERROR_RTDS());
    rc_zkr->SIGNAL_STATE_ERROR_NERASCEP().setValue_1bit(rc_zkr->STATE_ERROR_NERASCEP());
    rc_zkr->SIGNAL_STATE_ERROR_OSYCOUNT().setValue_1bit(rc_zkr->STATE_ERROR_OSYCOUNT());
    if (curr_state_zkr.sost==_otcep_unknow) rc_zkr->SIGNAL_STATE_OTCEP_UNKNOW().setValue_1bit(1);else
        rc_zkr->SIGNAL_STATE_OTCEP_UNKNOW().setValue_1bit(0);
    if (curr_state_zkr.sost==_otcep_free)   rc_zkr->SIGNAL_STATE_OTCEP_FREE().setValue_1bit(1);else
        rc_zkr->SIGNAL_STATE_OTCEP_FREE().setValue_1bit(0);
    if (curr_state_zkr.sost==_otcep_in)     rc_zkr->SIGNAL_STATE_OTCEP_IN().setValue_1bit(1);else
        rc_zkr->SIGNAL_STATE_OTCEP_IN().setValue_1bit(0);
}

int _find_step(tos_Zkr_DSO::t_zkr_pairs steps[],int size_steps,const tos_Zkr_DSO::t_zkr_state &prev_state,const tos_Zkr_DSO::t_zkr_state &curr_state)
{
    for (int i=0;i<size_steps;i++){
        if (!prev_state.likeTable(steps[i].prev_state)) continue;
        if (!curr_state.likeTable(steps[i].curr_state)) continue;
        return steps[i].cmd;
    }
    return _none;
}


void tos_Zkr_DSO::work(const QDateTime &T)
{
    prev_state_zkr=curr_state_zkr;
    int rtds=curr_state_zkr.rtds;
    if (rc_zkr->RTDS_USL_OR()){
        if ((rc_zkr->rtds_1()->STATE_SRAB()==1) || (rc_zkr->rtds_2()->STATE_SRAB()==1)) rtds=1;
    } else {
        if ((rc_zkr->rtds_1()->STATE_SRAB()==1) && (rc_zkr->rtds_2()->STATE_SRAB()==1)) rtds=1;
    }
    if ((rc_zkr->rtds_1()->STATE_SRAB()==0) && (rc_zkr->rtds_2()->STATE_SRAB()==0)) rtds=0;
    curr_state_zkr.rtds=rtds;
    curr_state_zkr.dso=alive_dso()->os_moved;
    if (trc->l_os.isEmpty())  curr_state_zkr.os_in=0; else curr_state_zkr.os_in=1;
    // ошибка ртдс
    //    if ((rtds==1)&&
    //            (trc->next_rc[_forw]!=nullptr)&&
    //            (trc->rc->STATE_BUSY()==MVP_Enums::free) &&
    //            (trc->next_rc[_forw]->rc->STATE_BUSY()==MVP_Enums::free)){
    //        rc_zkr->setSTATE_ERROR_RTDS(true);
    //    }
    //    if (rtds==0) rc_zkr->setSTATE_ERROR_RTDS(false);

    static t_zkr_pairs tos_zkr_steps[]={
        //sost;rtds;dso;os_in
        // начальное состояние
        { {_otcep_unknow,_xx,_xx     ,_xx } ,
          {_otcep_unknow, 0 ,_os_none, 0  } ,_sost2free},
        // нормальное выявление
        { {_otcep_free , 0 ,_os_none   ,0 } ,
          {_otcep_free , 0 ,_os2forw,   0  } ,_in},
        { {_otcep_free , 0 ,_os_none   ,0 } ,
          {_otcep_free , 1 ,_os2forw,   0  } ,_in},
        // выявление хвоста
        { {_otcep_in , 1 ,_os_none   ,_xx } ,
          {_otcep_in , 0 ,_os_none,   _xx } ,_otcep_end},
        // выявление хвоста
        { {_otcep_in , 0 ,_os_none   ,1 } ,
          {_otcep_in , 0 ,_os_none,   0 } ,_otcep_end},
        // нормальное движение вперед + дб
        { {_otcep_in , 1 ,_os_none   ,0 } ,
          {_otcep_in , 1 ,_os2forw,   0 } ,_check_end_in},
        // нормальное движение вперед
        { {_otcep_in , 1 ,_os_none   ,_xx } ,
          {_otcep_in , 1 ,_os2forw,   _xx  } ,_os_plus},
        // любое движение вперед
        { {_xx , _xx ,_os_none   ,_xx } ,
          {_xx , _xx ,_os2forw,   _xx  } ,_os_any_plus},
        // любое движение назад
        { {_xx , _xx ,_os_none   ,_xx } ,
          {_xx , _xx ,_os2back,   _xx  } ,_os_minus}
    };
    // ищем переход
    int cmd=_none;
    cmd= _find_step(tos_zkr_steps,sizeof(tos_zkr_steps)/sizeof(tos_zkr_steps[0]) ,prev_state_zkr,curr_state_zkr);
    switch (cmd) {
    case _in:
        newOtcep(T);
        curr_state_zkr.sost=_otcep_in;
        break;
    case _sost2free:
        curr_state_zkr.sost=_otcep_free;
        break;
    case _otcep_end:
        endOtcep(T);
        curr_state_zkr.sost=_otcep_free;
        break;
    case _os_plus:
        in_os(T);
        break;
    case _os_plus_baza:
        in_os(T);
        setOtcepBaza();
        break;
    case _check_end_in:
        check_end_in(T);
        break;
    case _os_any_plus:
        in_os(T);
        break;
    case _os_minus:
        out_os(T);
        break;
    default:
        break;
    }

    work_dso_tlg(T);

}

void tos_Zkr_DSO::work_dso_tlg(const QDateTime &T)
{
    dsp_pair->work(T);
    if (dsp_pair->sost_teleg==tos_DsoPair::_tlg_forw){
        if (dsp_pair->teleg_os.num>0){
            auto o=TOS->otcep_by_num(dsp_pair->teleg_os.num);
            if (o!=nullptr){
                int tlg=o->otcep->STATE_ZKR_TLG();
                tlg++;
                o->otcep->setSTATE_ZKR_TLG(tlg);
                if (tlg%2==0) o->otcep->setSTATE_ZKR_VAGON_CNT(tlg/2); else o->otcep->setSTATE_ZKR_VAGON_CNT(tlg/2+1);
            }
        }
    }
    if (dsp_pair->sost_teleg==tos_DsoPair::_tlg_back){
        if (dsp_pair->teleg_os.num>0){
            auto o=TOS->otcep_by_num(dsp_pair->teleg_os.num);
            if (o!=nullptr){
                int tlg=o->otcep->STATE_ZKR_TLG();
                tlg--;
                o->otcep->setSTATE_ZKR_TLG(tlg);
                if (tlg%2==0) o->otcep->setSTATE_ZKR_VAGON_CNT(tlg/2); else o->otcep->setSTATE_ZKR_VAGON_CNT(tlg/2+1);
            }
        }
    }
}

tos_DSO *tos_Zkr_DSO::alive_dso()
{
    if (!tdso[1][0]->dso->STATE_ERROR()) return tdso[1][0];
    return tdso[1][1];
}

void tos_Zkr_DSO::newOtcep(const QDateTime &T)
{
    curr_state_zkr.sost=_otcep_in;

    TOtcepDataOs os;
    os.zkr_num=TOS->zkr_id++;
    os.d=_forw;

    rc_zkr->setSTATE_ERROR_NERASCEP(false);
    rc_zkr->setSTATE_ERROR_OSYCOUNT(false);

    auto *otcep=TOS->getNewOtcep(trc);
    if (otcep!=nullptr){
        otcep->resetTracking();
        os.num=otcep->otcep->NUM();
        os.p=_pOtcepStartEnd;
        os.os_otcep=1;

        trc->l_otceps.push_back(os.num);

        otcep->otcep->setSTATE_DIRECTION(0);
        otcep->otcep->setSTATE_LOCATION(m_Otcep::locationOnSpusk);
        otcep->otcep->setSTATE_ZKR_BAZA(false);
        otcep->otcep->setSTATE_ZKR_PROGRESS(true);
        otcep->otcep->setSTATE_ZKR_OSY_CNT(1);
        otcep->otcep->setSTATE_ZKR_VAGON_CNT(1);
    } else {
        os.num=0;
        os.p=_pOtcepPartUnknow;
        os.os_otcep=0;
    }
    trc->l_os.push_back(os);
    cur_os=os;
    TOS->updateOtcepsOnRc(T);

}

void tos_Zkr_DSO::endOtcep(const QDateTime &T)
{
    cur_os=TOtcepDataOs();
    curr_state_zkr.sost=_otcep_free;
    if (!trc->l_os.isEmpty()){
        TOtcepDataOs &os=trc->l_os.last();
        os.p=_pOtcepStartEnd;
    }
    TOS->updateOtcepsOnRc(T);

}

void tos_Zkr_DSO::in_os(const QDateTime &T)
{
    TOtcepDataOs os=cur_os;
    os.p=_pOtcepPartUnknow;
    os.d=_forw;
    os.t=T;

    if (cur_os.num!=0){
        os.p=_pOtcepMiddle;
        os.os_otcep++;
        auto o=TOS->otcep_by_num(os.num);
        if (o!=nullptr){
            o->otcep->setSTATE_ZKR_OSY_CNT(os.os_otcep);
        }
        os.p=_pOtcepMiddle;
    }

    trc->l_os.push_back(os);
    cur_os=os;
    TOS->updateOtcepsOnRc(T);
}

void tos_Zkr_DSO::out_os(const QDateTime &T)
{
    cur_os=TOtcepDataOs();
    if (!trc->l_os.isEmpty()){
        TOtcepDataOs os=trc->l_os.last();
        trc->l_os.pop_back();
        if ((os.num!=0)&&(os.os_otcep==1)){
            TOS->resetTracking(os.num);
            auto o=TOS->otcep_by_num(os.num);
            if (o!=nullptr){
                if (rc_zkr->STATE_ROSPUSK()==1){
                    o->otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);
                }
            }
            curr_state_zkr.sost=_otcep_unknow;
        }
        if (!trc->l_os.isEmpty()) cur_os=trc->l_os.last();
        TOS->updateOtcepsOnRc(T);
    }
}
void tos_Zkr_DSO::check_end_in(const QDateTime &T)
{
    // неудачно расположены ртдс
    if (cur_os.os_otcep<4){
        in_os(T);
        return;
    }
    // если хоть один ртдс свободен - режем
    if ((rc_zkr->rtds_1()->STATE_SRAB()==0)||(rc_zkr->rtds_2()->STATE_SRAB()==0)){
        endOtcep(T);
        newOtcep(T);
        return;
    }
    in_os(T);
}


void tos_Zkr_DSO::setOtcepBaza()
{
    // неудачно расположены датчики
    // проверим что на первом точно ничего нет
    if ((alive_dso()!=nullptr)&&(alive_dso()->dso->STATE_SRAB()!=0)) return;
    if (!trc->l_os.isEmpty()){
        TOtcepDataOs os=trc->l_os.last();
        if ((os.num!=0)){
            auto o=TOS->otcep_by_num(os.num);
            if (o!=nullptr){
                o->otcep->setSTATE_ZKR_BAZA(1);
            }
        }
    }
}



void tos_Zkr_DSO::resetStates()
{
    curr_state_zkr.sost=_otcep_unknow;
    curr_state_zkr.rtds=-1;
    BaseWorker::resetStates();
    rc_zkr->setSTATE_ERROR_RTDS(false);
    rc_zkr->setSTATE_ERROR_NERASCEP(false);
    rc_zkr->setSTATE_ERROR_OSYCOUNT(false);
    rc_zkr->setSTATE_OTCEP_UNKNOW(false);
    rc_zkr->setSTATE_OTCEP_FREE(false);
    rc_zkr->setSTATE_OTCEP_IN(false);

}

