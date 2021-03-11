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
    // выставляем признак текущегог отцепа
    m_Otcep * cur_otcep=nullptr;
    foreach (auto *otcep, otceps->l_otceps) {
        if (!otcep->STATE_ENABLED()) continue;
        if (otcep->STATE_ZKR_S_IN()) cur_otcep=otcep;
    }
    if (cur_otcep==nullptr){
        foreach (auto *otcep, otceps->l_otceps) {
            if (!otcep->STATE_ENABLED()) continue;
            if (otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) {cur_otcep=otcep;break;}
        }
    }
    foreach (auto *otcep, otceps->l_otceps) {
        if (otcep!=cur_otcep) otcep->setSTATE_IS_CURRENT(0);
    }
    if (cur_otcep!=nullptr) cur_otcep->setSTATE_IS_CURRENT(1);

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
        s1.getBuffer()->setSizeData(sizeof(t_NewDescr));
        mO29[otcep]=s1;
        l<< s1;
        //        auto s2=otcep->SIGNAL_DATA();
        //        s2.setChanelType(109);
        //        mO2109[otcep]=s2;
        //        l<< s2;
    }
    // 14 сортир 15 от увк
    otceps->setSIGNAL_DATA_VAGON_0(otceps->SIGNAL_DATA_VAGON_0().innerUse());
    for (int i=0;i<MaxVagon;i++){
        SignalDescription p=SignalDescription(15,QString("vag%1").arg(i+1),0);
        p.acceptGtBuffer();
        p.getBuffer()->setSizeData(sizeof(tSlVagon));
        l_chanelVag.push_back(p);
        l<<p;
    }
    return l;
}
char __STATE_[]="STATE_";
char __STATE_D_[]="STATE_D_";
void OtcepsController::state2buffer()
{
    foreach (auto *otcep, otceps->l_otceps) {
        {
            t_NewDescr stored_Descr;
            memset(&stored_Descr,0,sizeof(stored_Descr));
            otcep->states2descr_ext(stored_Descr);
            if (otcep->RCS!=nullptr) stored_Descr.D.start  =otcep->RCS->SIGNAL_BUSY_DSO().chanelOffset()+1; else stored_Descr.D.start  =0;
            if (otcep->RCF!=nullptr) stored_Descr.D.end    =otcep->RCF->SIGNAL_BUSY_DSO().chanelOffset()+1; else stored_Descr.D.end  =0;
            auto &s=mO29[otcep];
            s.setBufferData(&stored_Descr,sizeof(stored_Descr));
        }

        //        {  Исключаю вариант - жутко тормозит с QString
        //            QString S;
        //            QVariantHash m;
        //            m["NUM"]=otcep->NUM();
        //            for (int idx = 0; idx < otcep->metaObject()->propertyCount(); idx++) {
        //                const QMetaProperty &metaProperty = otcep->metaObject()->property(idx);
        //                auto stateName=metaProperty.name();
        //                if (strncmp(stateName,__STATE_,sizeof(__STATE_)-1)!=0) continue;
        //                QString ss;
        //                ss=QString::fromLocal8Bit(&stateName[sizeof(__STATE_)-1]);
        //                QVariant V=metaProperty.read(otcep);
        //                m[ss]=V;
        //            }
        //            S=MVP_System::QVariantHashToQString_str(m);
        //            auto &s=mO2109[otcep];
        //            s.getBuffer()->A=S.toUtf8();
        //        }
    }

    //updateVagons();

    tSlVagon vagons0;
    memset(&vagons0,0,sizeof(vagons0));
    foreach (auto &s, l_chanelVag) {
        s.setValue_data(&vagons0,sizeof(vagons0));
    }
    int i=0;
    tSlVagon v;
    foreach (auto otcep, otceps->otceps()) {
        int kv=otcep->STATE_SL_VAGON_CNT();
        if (kv>0){
            for (int iv=0;iv<kv;iv++){
                if (iv<otcep->vVag.size()){
                    v=otcep->vVag[iv];
                } else {
                    memset(&v,0,sizeof(v));
                }

                if (i>=l_chanelVag.size()) break;
                v.NO=otcep->NUM();
                v.Id=otcep->STATE_ID_ROSP();
                v.SP=otcep->STATE_SP();
                v._tick=otcep->STATE_TICK();
                auto &s=l_chanelVag[i];
                s.setBufferData(&v,sizeof(v));
                //s.setValue_data(&v,sizeof(v));
                i++;
            }
        } else {
            if (i>=l_chanelVag.size()) break;
            memset(&v,0,sizeof(v));
            v.NO=otcep->NUM();
            v.Id=otcep->STATE_ID_ROSP();
            v.SP=otcep->STATE_SP();
            v._tick=otcep->STATE_TICK();
            auto &s=l_chanelVag[i];
            s.setBufferData(&v,sizeof(v));
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

bool OtcepsController::cmd_UPDATE_ALL(QString &acceptStr)
{
    foreach (m_Otcep*otcep, otceps->l_otceps) {
        otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
    }
    acceptStr="Обновление списка отцепов.";
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
            otcep_0->setSTATE_EXTNUM(otcep_1->STATE_EXTNUM());
            otcep_0->setSTATE_LOCATION(otcep_1->STATE_LOCATION());
            otcep_0->setSTATE_ENABLED(otcep_1->STATE_ENABLED());
            otcep->inc_tick();

        }
        otceps->l_otceps.last()->resetStates();
        updateVagons();
        acceptStr=QString("Отцеп %1 удален.").arg(N);
        return true;

    }
    acceptStr=QString("Ошибка удаления отцепа %1.").arg(m["N"]);
    return false;
}
m_Otcep* OtcepsController::inc_otcep(int N,int mar)
{
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        int ib=otceps->l_otceps.indexOf(otcep);
        for (int i=otceps->l_otceps.size()-1;i>=ib+1;i--){
            auto otcep_0=otceps->l_otceps[i-1];
            auto otcep_1=otceps->l_otceps[i];
            if ((otcep_0->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) break;
            if ((otcep_1->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) break;
            otcep_1->acceptSLStates(otcep_0);
            otcep_1->setSTATE_EXTNUM(otcep_0->STATE_EXTNUM());
            otcep_1->setSTATE_LOCATION(otcep_0->STATE_LOCATION());
            otcep_1->setSTATE_ENABLED(otcep_0->STATE_ENABLED());
            otcep_1->inc_tick();
        }
        otcep->resetStates();
        //if (n<otceps->l_otceps.size()-1) otceps->l_otceps[n]->acceptSLStates(otceps->l_otceps[n+1]);
        otcep->setSTATE_MAR(mar);
        otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);

        // добавляем 1 пустой вагон
        //otcep->vVag.push_back(tSlVagon());
        otcep->setSTATE_ENABLED(true);
        updateVagons();
    }
    return otcep;
}

m_Otcep *OtcepsController::inc_otcep_drobl(int N,int kv)
{
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        auto part=otcep->STATE_EXTNUMPART();
        if (part==0){
            part=1;
            otcep->setSTATE_EXTNUMPART(part);
            // проставляем EXTNUM
            int n=otceps->l_otceps.indexOf(otcep);
            if (n>=0){
                for (int i=n;i<otceps->l_otceps.size();i++){
                    auto otcep_1=otceps->l_otceps[i];
                    if (otcep_1->STATE_ENABLED()){
                        if (otcep_1->STATE_EXTNUM()==0) otcep_1->setSTATE_EXTNUM(otcep_1->NUM());
                    }
                }
            }
        }
        auto new_otcep=inc_otcep(N+1,otcep->STATE_MAR());
        if (new_otcep!=nullptr){
            part++;
            new_otcep->setSTATE_EXTNUMPART(part);
            new_otcep->setSTATE_EXTNUM(otcep->STATE_EXTNUM());
            otcep->setSTATE_SL_VAGON_CNT(kv);
            if (otcep->STATE_SL_OSY_CNT()>otcep->STATE_ZKR_OSY_CNT()) new_otcep->setSTATE_SL_OSY_CNT(otcep->STATE_SL_OSY_CNT()-otcep->STATE_ZKR_OSY_CNT());

            return new_otcep;
        }
    }
    return nullptr;
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
        if (otcep->NUM()>1){
            auto otcep1=otceps->otcepByNum(N-1);
            if (otcep1!=nullptr){
                if (otcep1->STATE_LOCATION()!=m_Otcep::locationOnPrib){
                    acceptStr=QString("Попытка добавить отцеп вне очереди. Отцеп %1.").arg(N);
                    return false;
                }
            }
        }
        inc_otcep(N,1);

        acceptStr=QString("Отцеп %1 добавлен.").arg(N);
        updateVagons();
        if (otcep->NUM()==1){
            auto ID_ROSP=otcep->STATE_ID_ROSP();
            if (ID_ROSP==0){
                time_t n;
                time(&n);
                uint32 r = n * 16;
                ID_ROSP=r;
                setNewID_ROSP(ID_ROSP);
            }
        }
        return true;
    }
    acceptStr=QString("Ошибка добавления отцепа %1.").arg(m["N"]);
    return false;
}

bool OtcepsController::cmd_SET_CUR_OTCEP(QMap<QString, QString> &m, QString &acceptStr)
{
    int N=m["SET_CUR_OTCEP"].toInt();
    auto otcep1=otceps->otcepByNum(N);
    if ((otcep1!=nullptr)&&(otcep1->STATE_ENABLED())) {
        // все белые до санут серыми
        bool befor=true;
        foreach (auto otcep, otceps->l_otceps) {
            if (otcep==otcep1) {
                befor=false;
            }
            if (befor){
                if ((otcep->STATE_ENABLED())&&(otcep->STATE_LOCATION()==m_Otcep::locationOnPrib)){
                    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                    otcep->setSTATE_GAC_ACTIVE(0);
                    otcep->setSTATE_ERROR(0);
                }
            } else {
                if (otcep->STATE_ENABLED()) {
                    otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);
                    otcep->setSTATE_GAC_ACTIVE(0);
                    otcep->setSTATE_ERROR(0);
                }

            }
        }
        acceptStr=QString("Отцеп %1 стал текущим.").arg(N);
        foreach (m_Otcep*otcep, otceps->l_otceps) {
            otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
        }
        updateVagons();
        return true;
    }

    acceptStr=QString("Ошибка установки текущего отцепа %1.").arg(m["N"]);
    return false;
}

bool OtcepsController::cmd_SET_OTCEP_STATE(QMap<QString, QString> &m, QString &acceptStr)
{
    if (!m["N"].isEmpty()){

        int N=m["N"].toInt();
        auto otcep=otceps->otcepByNum(N);
        if (otcep!=nullptr){
            if (((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)||(otcep->STATE_LOCATION()==m_Otcep::locationOnPark))
                    &&(!otcep->STATE_IS_CURRENT())) {
                acceptStr=QString("Отцеп %1 попытка изменить выявленный отцеп.").arg(N);
                return false;
            }
            bool ex_change=false;
            foreach (QString key, m.keys()) {
                if ((key=="CMD") || (key=="N")) continue;
                QString stateName="STATE_"+key;
                QVariant V;
                V=m[key];

                for (int idx = 0; idx < otcep->metaObject()->propertyCount(); idx++) {
                    QMetaProperty metaProperty = otcep->metaObject()->property(idx);
                    if (metaProperty.name()!=stateName) continue;
                    QVariant V1=otcep->property(qPrintable(stateName));
                    if (V1!=V){
                        qDebug()<< stateName << V1.toString() << V.toString();
                        ex_change=true;
                        //otcep->setProperty(qPrintable(stateName),V);
                        metaProperty.write(otcep,V);
                    }

                }
            }
            if (otcep->NUM()==1){
                if (m.keys().contains("ENABLED")){
                    if (m["ENABLED"]=="1"){
                        auto ID_ROSP=otcep->STATE_ID_ROSP();
                        if (ID_ROSP==0){
                            time_t n;
                            time(&n);
                            uint32 r = n * 16;
                            ID_ROSP=r;
                            setNewID_ROSP(ID_ROSP);
                        }
                    }
                }
            }

            if (ex_change){
                acceptStr=QString("Отцеп %1 свойства изменены.").arg(N);
                otcep->inc_tick();
            }
//            updateVagons();
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

            int NV=m["NV"].toInt();
            if (NV>otcep->vVag.size()){
                int add_cnt=NV-otcep->vVag.size();
                for (int i=0;i<add_cnt;i++){
                    otcep->vVag.push_back(tSlVagon());
                }
            }
            otcep->vVag[NV-1]=v;
            //otceps->vagons[v.IV-1]=v;
            acceptStr=QString("Отцеп %1 добавлен ваг. %2 .").arg(m["N"]).arg(m["IV"]);
            updateVagons();
            return true;
        }
    }
    acceptStr=QString("Ошибка добавления: Отцеп %1 добавлен ваг. %2 .").arg(m["N"]).arg(m["IV"]);
    return false;
}

void OtcepsController::resetLiveOtceps()
{
    foreach (auto otcep, otceps->otceps()) {
        if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)
            otcep->resetStates();
        //otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
    }

}

void OtcepsController::finishLiveOtceps()
{
    foreach (auto otcep, otceps->otceps()) {
        if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)
            otcep->setSTATE_LOCATION(m_Otcep::locationUnknow); // нужно для старых
    }
}

void OtcepsController::setNewID_ROSP(quint32 ID_ROSP)
{
    foreach (auto otcep, otceps->otceps()) {
        if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib){
            otcep->setSTATE_ID_ROSP(ID_ROSP);
            otcep->inc_tick();
        }
    }
}

void OtcepsController::updateVagons()
{
    // инкремент тика
    foreach (auto otcep, otceps->otceps()) {
        if (otcep->STATE_ENABLED()==1){
            otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
        }
    }
}

