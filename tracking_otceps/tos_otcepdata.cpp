#include "tos_otcepdata.h"

#include "trackingotcepsystem.h"
#include "tos_speedcalc.h"
#include <QMetaProperty>

tos_OtcepData::tos_OtcepData(TrackingOtcepSystem *parent, m_Otcep *otcep) : BaseObject(parent)
{
    this->TOS=parent;
    this->otcep=otcep;
    otcep->tos=this;

    setObjectName(QString("OtcepData %1").arg(otcep->NUM()));
}

void tos_OtcepData::resetStates()
{
    //otcep->resetStates();
    //снимаем только параметры TOS
    otcep->setSTATE_CHANGE_COUNTER(0);
    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
    otcep->setSTATE_MAR_F(0);
    otcep->setSTATE_DIRECTION(0);
    otcep->setSTATE_NAGON(0);
    otcep->setSTATE_ERROR(0);

    otcep->setSTATE_VAGON_CNT(0);
    otcep->setSTATE_OSY_CNT(0);
    otcep->setSTATE_VES(0);
    otcep->setSTATE_BAZA(0);
    otcep->setSTATE_LEN(0);

    otcep->setSTATE_ZKR_PROGRESS(false);
    otcep->setSTATE_ZKR_VAGON_CNT(0);
    otcep->setSTATE_ZKR_OSY_CNT(0);
    otcep->setSTATE_ZKR_BAZA(0);

    otcep->setSTATE_KZP_OS(m_Otcep::kzpUnknow);

    otcep->setSTATE_LEN_BY_VAGON(0);
    otcep->setSTATE_LEN_BY_RC_MIN(0);
    otcep->setSTATE_LEN_BY_RC_MAX(0);

    otcep->setSTATE_V(_undefV_);
    otcep->setSTATE_V_RC(_undefV_);
    otcep->setSTATE_V_ARS(_undefV_);
    otcep->setSTATE_V_KZP(_undefV_);
    otcep->setSTATE_V_DISO(_undefV_);


    otcep->setSTATE_V_IN_1(_undefV_);
    otcep->setSTATE_V_IN_2(_undefV_);
    otcep->setSTATE_V_IN_3(_undefV_);
    otcep->setSTATE_V_OUT_1(_undefV_);
    otcep->setSTATE_V_OUT_2(_undefV_);
    otcep->setSTATE_V_OUT_3(_undefV_);

    otcep->RCS=nullptr;
    otcep->RCF=nullptr;
    otcep->vBusyRc.clear();

    dos_RCS=nullptr;
    dos_RCF=nullptr;
    _stat_kzp_d=0;
    _stat_kzp_t=QDateTime();
    out_tp_t=QDateTime();;
    out_tp_rc=nullptr;
    out_tp_v=_undefV_;

    setSTATE_V_RCF_IN(_undefV_);
    setSTATE_V_RCF_OUT(_undefV_);

}

void tos_OtcepData::setOtcepSF(m_RC *rcs, m_RC *rcf,const QDateTime &T)
{
    if ((otcep->RCS!=rcs)&&(otcep->RCF!=rcf)){
        emit TOS->otcep_rcsf_change(otcep,0,otcep->RCS,rcs,T,QDateTime());
        emit TOS->otcep_rcsf_change(otcep,1,otcep->RCF,rcf,T,QDateTime());
        otcep->RCS=rcs;
        otcep->RCF=rcf;
        dtRCS=QDateTime();
        dtRCF=QDateTime();
        otcep->setSTATE_V_RC(_undefV_);
        otcep->setBusyRC();
        TOS->updateOtcepsOnRc();
    } else
        if (otcep->RCS!=rcs){
            if ((rcs!=nullptr)&&(rcs->next_rc[1]==otcep->RCS)){
                qint64 ms=dtRCS.msecsTo(T);
                qreal V=tos_SpeedCalc::calcV(ms,otcep->RCS->LEN());
                otcep->setSTATE_V_RC(V);
                emit TOS->otcep_rcsf_change(otcep,0,otcep->RCS,rcs,T,dtRCS);
                dtRCS=T;
            } else {
                dtRCS=QDateTime();
                otcep->setSTATE_V_RC(_undefV_);
                emit TOS->otcep_rcsf_change(otcep,0,otcep->RCS,rcs,T,dtRCS);
            }
            otcep->RCS=rcs;
            otcep->setBusyRC();
            TOS->updateOtcepsOnRc();
        } else
            if (otcep->RCF!=rcf){
                if ((rcf!=nullptr)&&(rcf->next_rc[1]==otcep->RCF)){
                    qint64 ms=dtRCF.msecsTo(T);
                    qreal V=tos_SpeedCalc::calcV(ms,otcep->RCF->LEN());
                    otcep->setSTATE_V_RC(V);
                    emit TOS->otcep_rcsf_change(otcep,1,otcep->RCF,rcf,T,dtRCF);
                    dtRCF=T;
                } else {
                    dtRCF=QDateTime();
                    emit TOS->otcep_rcsf_change(otcep,1,otcep->RCF,rcf,T,dtRCF);
                }
                otcep->RCF=rcf;
                otcep->setBusyRC();
                TOS->updateOtcepsOnRc();
            }
}


qreal tos_OtcepData::STATE_V() const
{
    if (otcep->STATE_V_ARS()!=_undefV_) return otcep->STATE_V_ARS();
    if (otcep->STATE_V_KZP()!=_undefV_) return otcep->STATE_V_KZP();
    if (otcep->STATE_V_DISO()!=_undefV_) return otcep->STATE_V_DISO();
    return otcep->STATE_V_RC();
}

void tos_OtcepData::calcLenByRc()
{
    qreal l=0;
    foreach (auto rc, otcep->vBusyRc) {
        if (
                (rc->LEN()!=0) &&
                (rc->next_rc[0]!=nullptr) &&
                (rc->next_rc[1]!=nullptr) &&
                (rc->rcs->useRcTracking)&&
                (rc->next_rc[0]->rcs->useRcTracking)
                /*(qobject_cast<m_RC_Gor_Park*>(rc)==nullptr) &&
                        (qobject_cast<m_RC_Gor_Park*>(rc->next_rc[0])==nullptr) &&
                        (qobject_cast<m_RC_Gor_ZKR*>(rc)==nullptr)*/)
        {
            l=l+rc->LEN();
        } else
        {
            return;
        }
    }
    if ((otcep->STATE_LEN_BY_RC_MIN()==0)||(l<otcep->STATE_LEN_BY_RC_MIN()))
        otcep->setSTATE_LEN_BY_RC_MIN(l);
    if ((otcep->STATE_LEN_BY_RC_MAX()==0)||(l>otcep->STATE_LEN_BY_RC_MAX()))
        otcep->setSTATE_LEN_BY_RC_MAX(l);
}

void tos_OtcepData::state2buffer()
{
    if (otcep->SIGNAL_DATA().chanelType()==9){
        t_Descr stored_Descr;
        memset(&stored_Descr,0,sizeof(stored_Descr));
        stored_Descr.num=   otcep->NUM();                   ; // Номер отцепа 1-255 Живет в течении роспуска одного
        stored_Descr.mar=otcep->STATE_MAR();         ;
        stored_Descr.mar_f=otcep->STATE_MAR_F();
        if (otcep->RCS) stored_Descr.start=otcep->RCS->SIGNAL_BUSY().chanelOffset()+1;else stored_Descr.start=0;
        if (otcep->RCF) stored_Descr.end=otcep->RCF->SIGNAL_BUSY().chanelOffset()+1;else stored_Descr.end=0;
        stored_Descr.ves=otcep->STATE_VES();              ; // Вес отцепа в тоннах
        stored_Descr.osy=otcep->STATE_OSY_CNT();         ; // Длинна ( в осях)
        stored_Descr.len=otcep->STATE_VAGON_CNT();        ; // Длинна ( в вагонах)
        stored_Descr.baza=otcep->STATE_BAZA();           ; // Признак длиннобазности
        stored_Descr.nagon=otcep->STATE_NAGON();          ; // Признак нагона
        if (otcep->STATE_LOCATION()!=m_Otcep::locationOnSpusk)
            stored_Descr.end_slg=0; else
            stored_Descr.end_slg=1;// Признак конца слежения (по последней РЦ на путях)

        stored_Descr.err=otcep->STATE_ERROR();           ; // Признак неперевода стрелки
        stored_Descr.dir=otcep->STATE_DIRECTION();        ; // Направление
        stored_Descr.V_rc=otcep->STATE_V_RC()*10;      ; // Скорость по РЦ
        //            stored_Descr.V_zad  ; // Скорость заданная
        //            stored_Descr.Stupen ; // Ступень торможения
        //            stored_Descr.osy1   ; // Длинна ( в осях)
        //            stored_Descr.osy2   ; // Длинна ( в осях)
        //            stored_Descr.V_zad2 ; // Скорость заданная 2TP
        //            stored_Descr.V_zad3 ; // Скорость заданная  3TP
        //            stored_Descr.pricel ;
        //            stored_Descr.old_num;
        //            stored_Descr.old_mar;
        //            stored_Descr.U_len  ;
        stored_Descr.vagon=otcep->STATE_SL_VAGON_CNT();   ;
        stored_Descr.V_out=otcep->STATE_V_OUT_1()*10 ;
        stored_Descr.V_out=otcep->STATE_V_IN_2()*10 ;
        stored_Descr.V_out2=otcep->STATE_V_OUT_2()*10 ;
        stored_Descr.V_in3=otcep->STATE_V_IN_3()*10  ;
        stored_Descr.V_out3=otcep->STATE_V_OUT_3()*10 ;
        stored_Descr.Id=otcep->STATE_ID_ROSP();
        //            stored_Descr.st     ;
        stored_Descr.ves_sl=otcep->STATE_SL_VES();
        //            stored_Descr.r_mar  ;
        //            stored_Descr.t_ot[3]; // 0- растарможка 1-4 ступени максимал ступень работы замедлителя
        //            stored_Descr.r_a[3] ; // 0-автомат режим ручного вмешательсва
        stored_Descr.V_in=otcep->STATE_V_IN_1()*10 ; // Cкорость входа 1 ТП
        //            stored_Descr.Kzp    ; // КЗП по расчету Антона
        //            stored_Descr.v_rosp ; // Скорость расформирования   - норма/быстро/медленно - 0/1/2
        //            stored_Descr.flag_ves; // Работоспособность весомера - да/нет/ - 0/1
        //            stored_Descr.flag_r  ; // Признак ручной установки скорости
        //            stored_Descr.FirstVK ;
        //            stored_Descr.LastVK  ;
        //            stored_Descr.addr_tp[3]; // Занятый замедлитель
        //            stored_Descr.v_rosp1 ; // Скорость расформирования   - норма/быстро/медленно - 0/1/2
        //            stored_Descr.p_rzp   ; // Признак выше ПТП
        //                   stored_Descr.VrospZ;
        //                   stored_Descr.VrospF;
        //                   stored_Descr.V_zad2_S ; // Скорость заданная 2TP
        otcep->SIGNAL_DATA().setBufferData(&stored_Descr,sizeof(stored_Descr));
    }
    QString S;
    if (otcep->SIGNAL_DATA().chanelType()==109){
        for (int idx = 0; idx < otcep->metaObject()->propertyCount(); idx++) {
            QMetaProperty metaProperty = otcep->metaObject()->property(idx);
            QString stateName=metaProperty.name();
            if (stateName.indexOf("STATE_")!=0) continue;
            if (!S.isEmpty()) S=S+";";
            stateName.remove(0,6);
            QVariant V=metaProperty.read(otcep);
            S=stateName+"="+V.toString();
        }
        otcep->SIGNAL_DATA().getBuffer()->A=S.toUtf8();
    }
}



void tos_OtcepData::setCurrState()
{

    otcep->tos->rc_tracking_comleted[0]=false;
    otcep->tos->rc_tracking_comleted[1]=false;
}



