#include "otcepscontroller.h"
#include <QMetaProperty>
#include "mvp_system.h"

OtcepsController::OtcepsController(QObject *parent,m_Otceps *otceps) : BaseWorker(parent)
{
    this->otceps=otceps;
}

void OtcepsController::resetStates()
{
    BaseWorker::resetStates();
    otceps->resetStates();
}

void OtcepsController::work(const QDateTime &)
{

}

void OtcepsController::validation(ListObjStr *l) const
{
    BaseWorker::validation(l);
}


QList<SignalDescription> OtcepsController::acceptOutputSignals()
{
    QList<SignalDescription> l;
    // рассыка в обоих форматах
    foreach (auto *otcep, otceps->l_otceps) {
        otcep->setSIGNAL_DATA( otcep->SIGNAL_DATA().innerUse());
        auto s1=otcep->SIGNAL_DATA();
        s1.setChanelType(9);
        mO29[otcep]=s1;
        l<< s1;
        auto s2=otcep->SIGNAL_DATA();
        s2.setChanelType(109);
        mO2109[otcep]=s2;
        l<< s2;
    }
    // 14 сортир 15 от увк
    otceps->setSIGNAL_DATA_VAGON_0(otceps->SIGNAL_DATA_VAGON_0().innerUse());
    for (int i=0;i<MaxVagon;i++){
        SignalDescription p=SignalDescription(15,QString("vag%1").arg(i+1),0);
        l_chanelVag.push_back(p);
        l<<p;
    }
    return l;
}

void OtcepsController::state2buffer()
{
    foreach (auto *otcep, otceps->l_otceps) {
        {
            t_Descr stored_Descr;
            memset(&stored_Descr,0,sizeof(stored_Descr));
            if (otcep->STATE_ENABLED()){
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
                stored_Descr.V_zad = otcep->STATE_V_ZAD_1()==_undefV_ ? 65535 :otcep->STATE_V_ZAD_1()*10; // Скорость заданная
                stored_Descr.Stupen=otcep->STATE_SL_STUPEN() ; // Ступень торможения
                //            stored_Descr.osy1   ; // Длинна ( в осях)
                //            stored_Descr.osy2   ; // Длинна ( в осях)
                stored_Descr.V_zad2= otcep->STATE_V_ZAD_2()==_undefV_ ? 65535 :otcep->STATE_V_ZAD_2()*10;  // Скорость заданная 2TP
                stored_Descr.V_zad3= otcep->STATE_V_ZAD_3()==_undefV_ ? 65535 :otcep->STATE_V_ZAD_3()*10; ; // Скорость заданная  3TP
                //            stored_Descr.pricel ;
                //            stored_Descr.old_num;
                //            stored_Descr.old_mar;
                stored_Descr.U_len =otcep->STATE_SL_LEN() ;
                stored_Descr.vagon=otcep->STATE_SL_VAGON_CNT();   ;
                stored_Descr.V_out=otcep->STATE_V_OUT_1()*10 ;
                stored_Descr.V_out=otcep->STATE_V_IN_2()*10 ;
                stored_Descr.V_out2=otcep->STATE_V_OUT_2()*10 ;
                stored_Descr.V_in3=otcep->STATE_V_IN_3()*10  ;
                stored_Descr.V_out3=otcep->STATE_V_OUT_3()*10 ;
                stored_Descr.Id=otcep->STATE_ID_ROSP();
                stored_Descr.st =otcep->STATE_SL_STUPEN()     ;
                stored_Descr.ves_sl=otcep->STATE_SL_VES();
                //            stored_Descr.r_mar  ;
                for (int i=0;i<3;i++){
                    stored_Descr.t_ot[i]=otcep->STATE_OT_RA(0,i); // 0- растарможка 1-4 ступени максимал ступень работы замедлителя
                    stored_Descr.r_a[i]=otcep->STATE_OT_RA(1,i); ; // 0-автомат режим ручного вмешательсва
                }
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

                auto &s=mO29[otcep];
                s.setBufferData(&stored_Descr,sizeof(stored_Descr));
            }
        }

        {
            QString S;
            QVariantHash m;
            m["NUM"]=otcep->NUM();
            for (int idx = 0; idx < otcep->metaObject()->propertyCount(); idx++) {
                QMetaProperty metaProperty = otcep->metaObject()->property(idx);
                QString stateName=metaProperty.name();
                if (stateName.indexOf("STATE_")!=0) continue;
                if (stateName.indexOf("STATE_D_")==0) continue;
                stateName.remove(0,6);
                QVariant V=metaProperty.read(otcep);
                m[stateName]=V;
            }
            S=MVP_System::QVariantHashToQString_str(m);
            auto &s=mO2109[otcep];
            s.getBuffer()->A=S.toUtf8();
        }
    }

    //updateVagons();

    tSlVagon vagons0;
    memset(&vagons0,0,sizeof(vagons0));
    foreach (auto &s, l_chanelVag) {
        s.setValue_data(&vagons0,sizeof(vagons0));
    }
    int i=0;
    foreach (auto otcep, otceps->otceps()) {
        for (tSlVagon &v: otcep->vVag) {
            if (i>=l_chanelVag.size()) break;
            v.NO=otcep->NUM();
            v.Id=otcep->STATE_ID_ROSP();
            v.SP=otcep->STATE_SP();
            auto &s=l_chanelVag[i];
            s.setBufferData(&v,sizeof(v));
            //s.setValue_data(&v,sizeof(v));
            i++;
        }
    }

//    for (int i=0;i<MaxVagon;i++){
//        // рассылаем в старом
//        //if (otceps->TYPE_DESCR()==0){
//        otceps->chanelVag[i].getBuffer()->A.resize(sizeof(otceps->vagons[i]));
//        memcpy(otceps->chanelVag[i].getBuffer()->A.data(),&otceps->vagons[i],sizeof(otceps->vagons[i]));
//        //        if (otceps->TYPE_DESCR()==1){
//        //            QVariantHash m=tSlVagon2Map(otceps->vagons[i]);
//        //            QString S=MVP_System::QVariantHashToQString_str(m);
//        //            otceps->chanelVag[i]->A=S.toUtf8();
//        //        }
//    }
}

bool OtcepsController::cmd_CLEAR_ALL(QString &acceptStr)
{

    otceps->resetStates();
    updateVagons();
    acceptStr="Сброс списка отцепов.";
    return true;
}

bool OtcepsController::cmd_ACTIVATE_ALL(QString &acceptStr)
{
    foreach (m_Otcep*otcep, otceps->l_otceps) {
        if (otcep->STATE_MAR()>0) otcep->setSTATE_ENABLED(true);
    }
    acceptStr="Активация списка отцепов.";
    return true;
}

bool OtcepsController::cmd_DEL_OTCEP(QMap<QString, QString> &m, QString &acceptStr)
{
    int N=m["DEL_OTCEP"].toInt();
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        if ((otcep->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)){
            acceptStr=QString("Попытка удалить выявленный отцеп. Отцеп %1.").arg(N);
            return false;
        }
        int n=otceps->l_otceps.indexOf(otcep);
        for (int i=n;i<otceps->l_otceps.size()-1;i++){
            auto otcep_0=otceps->l_otceps[i];
            auto otcep_1=otceps->l_otceps[i+1];
            otcep_0->acceptSLStates(otcep_1);
            otcep_0->setSTATE_LOCATION(otcep_1->STATE_LOCATION());
            otcep_0->setSTATE_ENABLED(otcep_1->STATE_ENABLED());

        }
        otceps->l_otceps.last()->resetStates();
        updateVagons();
        acceptStr=QString("Отцеп %1 удален.").arg(N);
        return true;

    }
    acceptStr=QString("Ошибка удаления отцепа %1.").arg(m["N"]);
    return false;
}

bool OtcepsController::cmd_INC_OTCEP(QMap<QString, QString> &m, QString &acceptStr)
{
    int N=m["INC_OTCEP"].toInt();
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        if ((otcep->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)){
            acceptStr=QString("Попытка добавить отцеп перед выявленным. Отцеп %1.").arg(N);
            return false;
        }
        int ib=otceps->l_otceps.indexOf(otcep);
        for (int i=otceps->l_otceps.size()-1;i>=ib+1;i--){
            auto otcep_0=otceps->l_otceps[i-1];
            auto otcep_1=otceps->l_otceps[i];
            if ((otcep_0->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) break;
            if ((otcep_1->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) break;
            otcep_1->acceptSLStates(otcep_0);
            otcep_1->setSTATE_LOCATION(otcep_0->STATE_LOCATION());
            otcep_1->setSTATE_ENABLED(otcep_0->STATE_ENABLED());
        }
        otcep->resetStates();
        //if (n<otceps->l_otceps.size()-1) otceps->l_otceps[n]->acceptSLStates(otceps->l_otceps[n+1]);
        otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);
        otcep->setSTATE_ENABLED(true);
        acceptStr=QString("Отцеп %1 добавлен.").arg(N);
        updateVagons();
        return true;
    }
    acceptStr=QString("Ошибка добавления отцепа %1.").arg(m["N"]);
    return false;
}
bool OtcepsController::cmd_SET_OTCEP_STATE(QMap<QString, QString> &m, QString &acceptStr)
{
    if (!m["N"].isEmpty()){

        int N=m["N"].toInt();
        auto otcep=otceps->otcepByNum(N);
        if (otcep!=nullptr){
            foreach (QString key, m.keys()) {
                if ((key=="CMD") || (key=="N")) continue;
                QString stateName="STATE_"+key;
                QVariant V;
                V=m[key];

                for (int idx = 0; idx < otcep->metaObject()->propertyCount(); idx++) {
                    QMetaProperty metaProperty = otcep->metaObject()->property(idx);
                    if (metaProperty.name()!=stateName) continue;
                    QVariant V1=otcep->property(qPrintable(stateName));
                    if (V1!=V)
                        qDebug()<< stateName << V1.toString() << V.toString();
                    //otcep->setProperty(qPrintable(stateName),V);
                    metaProperty.write(otcep,V);

                }
            }
            acceptStr=QString("Отцеп %1 свойства изменены.").arg(N);
            return true;
        }
    }
    acceptStr=QString("Ошибка изменения свойств отцепа %1.").arg(m["N"]);
    return false;
}

bool OtcepsController::cmd_ADD_OTCEP_VAG(QMap<QString, QString> &m, QString &acceptStr)
{
    if (!m["N"].isEmpty()){
        int N=m["N"].toInt();
        auto otcep=otceps->otcepByNum(N);
        if (otcep!=nullptr){
            QVariantHash mv;
            foreach (QString key, m.keys()) {
                mv[key]=m[key];
            }
            tSlVagon v=Map2tSlVagon(mv);
            if (v.Id!=otcep->STATE_ID_ROSP()) {
                acceptStr=QString("Отцеп %1 ваг. %2 v.Id!=otcep->STATE_ID_ROSP().").arg(m["N"]).arg(m["IV"]);
                return false;
            }
            if (v.NO!=otcep->NUM()) {
                acceptStr=QString("Отцеп %1 ваг. %2 v.NO!=otcep->NUM().").arg(m["N"]).arg(m["IV"]);
                return false;
            };
            if ((v.IV<=0)||(v.IV>MaxVagon)) {
                acceptStr=QString("Отцеп %1 ваг. %2 (v.IV<=0)||(v.IV>MaxVagon).").arg(m["N"]).arg(m["IV"]);
                return false;
            };

            //                if (otcep->vVag.isEmpty()){
            //                    if (v.IV!=1) {
            //                        qDebug()<<"addOtcepVag: v.IV!=1";
            //                        return false;
            //                    };
            //                }else {
            //                    if (otcep->vVag.last().IV+1!=v.IV) {
            //                        qDebug()<<"addOtcepVag: otcep->vVag.last().IV+1!=v.IV";
            //                        return false;
            //                    };
            //                }
            otcep->vVag.push_back(v);
            //otceps->vagons[v.IV-1]=v;
            acceptStr=QString("Отцеп %1 добавлен ваг. %2 .").arg(m["N"]).arg(m["IV"]);
            updateVagons();
            return true;
        }
    }
    acceptStr=QString("Ошибка добавления: Отцеп %1 добавлен ваг. %2 .").arg(m["N"]).arg(m["IV"]);
    return false;
}

void OtcepsController::updateVagons()
{

}
