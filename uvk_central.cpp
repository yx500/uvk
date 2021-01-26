#include "uvk_central.h"
#include <QFileInfo>
#include "do_message.hpp"
#include "tos_system_dso.h"

int testMode=0;

class IUVKGetNewOtcep :public IGetNewOtcep
{
public:
    IUVKGetNewOtcep(UVK_Central *uvk){this->uvk=uvk;}
    virtual int getNewOtcep(m_RC*rc){return uvk->getNewOtcep(rc);}
    UVK_Central *uvk;
};


UVK_Central::UVK_Central(QObject *parent) : QObject(parent)
{

}

bool UVK_Central::init(QString fileNameIni)
{
    if (QFileInfo(fileNameIni).exists()){
        qDebug() << "ini load:" << QFileInfo(fileNameIni).absoluteFilePath();
        QSettings settings(fileNameIni,QSettings::IniFormat);
        fileNameModel=settings.value("main/model","./M.xml").toString();
        //        trackingType=settings.value("main/tracking_type",1).toInt();
        testMode=settings.value("test/mode",0).toInt();
    } else {
        {
            QSettings settings(fileNameIni,QSettings::IniFormat);
            settings.setValue("main/model","./M.xml");
            //            settings.setValue("main/tracking_type",1);
            fileNameIni=settings.fileName();
            settings.setValue("test/mode","0");

        }
        qDebug() << "ini created:" << QFileInfo(fileNameIni).absoluteFilePath();
        return false;
    }

    if (!QFileInfo(fileNameModel).exists()){
        qFatal("file not found %s",QFileInfo(fileNameModel).absoluteFilePath().toStdString().c_str());
        return false;
    }
    udp=new GtBuffers_UDP_D2();
    MVP.setGetGtBufferInterface(udp);
    if (fileNameModel.isEmpty()) fileNameModel="./M.xml";
    QObject *O=MVP.loadObject(fileNameModel);
    GORKA=qobject_cast<ModelGroupGorka *>(O);
    if (!GORKA){
        qFatal("class ModelGroupGorka not found");
        return false;
    }
    QList<m_Otceps *> l_otceps=GORKA->findChildren<m_Otceps*>();
    if (l_otceps.isEmpty()){
        qFatal("class m_Otceps not found");
        return false;
    }
    qDebug() << "model load:" << QFileInfo(fileNameModel).absoluteFilePath();

    l_zkr=GORKA->findChildren<m_RC_Gor_ZKR*>();

    otcepsController=new OtcepsController(this,l_otceps.first());

    trackingType=_tos_dso;
    TOS=new tos_System_DSO(this);
    IUVKGetNewOtcep *uvkGetNewOtcep=new IUVKGetNewOtcep(this);
    TOS->setIGetNewOtcep(uvkGetNewOtcep);
    TOS->makeWorkers(GORKA);

    GAC=new GtGac(this,GORKA);

    CMD=new GtCommandInterface(this,udp);
    CMD->setSRC_ID("UVK");

    if (!acceptBuffers())
    {
        return false;
    }

    GORKA->resetStates();
    TOS->resetStates();
    GAC->resetStates();
    otcepsController->resetStates();


    return validation();


}

void UVK_Central::start()
{
    connect(GAC,&GtGac::uvk_command,this,&UVK_Central::gac_command);

    connect(CMD,&GtCommandInterface::recv_cmd,this,&UVK_Central::recv_cmd);
    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,&UVK_Central::work);
    timer->start(100);
    // перебрасываем все каналы на себя
    foreach (GtBuffer *B, udp->allBuffers()) {
        if (!B->static_mode){
            connect(B,&GtBuffer::bufferChanged,this,&UVK_Central::work);
        }
    }
    maxOtcepCurrenRospusk=0;
    QString tmpstr;
    cmd_setPutNadvig(1,tmpstr);
    cmd_setRegim(0,tmpstr);
    sendBuffers();
    qDebug() << "uvk started ";
}

void UVK_Central::err( QString errstr)
{

    errLog.push_back(errstr);
}

bool UVK_Central::validation()
{
    ListObjStr l;
    l.clear();
    GORKA->validation(&l);
    if (l.contains_error){
        err("GORKA errors:");
        errLog.append(l.toStringList());
        return false;
    }
    l.clear();
    TOS->validation(&l);
    if (l.contains_error){
        err("TOS errors:");
        errLog.append(l.toStringList());
        return false;
    }
    l.clear();
    GAC->validation(&l);
    if (l.contains_error){
        err("GAC errors:");
        errLog.append(l.toStringList());
        return false;
    }
    return true;
}



bool UVK_Central::acceptBuffers()
{
    bool noerr=true;
    QList<SignalDescription> l;
    l+=this->acceptOutputSignals();
    l+=otcepsController->acceptOutputSignals();
    l+=TOS->acceptOutputSignals();
    l+=GAC->acceptOutputSignals();


    // закрываем буфера на прием
    foreach (auto p, l) {
        if (p.isEmpty())continue;
        if (l_out_buffers.indexOf(p.getBuffer())<0) {
            l_out_buffers.push_back(p.getBuffer());
            p.getBuffer()->static_mode=true;
        }
    }
    for (int i=0;i<l.size();i++){
        SignalDescription p1=l[i];
        if (p1.isEmpty()) continue;
        for (int j=0;j<l.size();j++){
            if (i==j) continue;
            SignalDescription p2=l[j];

            if ((p1.chanelType()==p2.chanelType())&&
                    (p1.chanelName()==p2.chanelName()) &&
                    (p1.chanelOffset()==p2.chanelOffset()))
            {
                err(QString("signal use more than 1 worker %1").arg(p1.toString()));
                noerr=false;
            }
        }
    }

    //    QList<BaseObject*> lo=GORKA->findChildren<BaseObject*>();

    //    foreach (BaseObject *b, lo) {
    //        // собираем буфера
    //        for (int idx = 0; idx < b->metaObject()->propertyCount(); idx++) {
    //            QMetaProperty metaProperty = b->metaObject()->property(idx);
    //            int type = metaProperty.userType();
    //            if (type == qMetaTypeId<SignalDescription>()){
    //                QVariant val=metaProperty.read(b);
    //                SignalDescription p = val.value<SignalDescription>();
    //                if (p.isEmpty())continue;
    //                if (p.isInnerUse()) {
    //                    if (l_out_buffers.indexOf(p.getBuffer())<0) {
    //                        l_out_buffers.push_back(p.getBuffer());
    //                        p.getBuffer()->static_mode=true;
    //                    }
    //                }
    //            }
    //        }
    //    }
    foreach (auto b, udp->allBuffers()) {
        if (!b->static_mode)
            qDebug()<< "use in buffer " <<b->getType() << b->objectName();
    }
    foreach (auto b, l_out_buffers) {
        b->sost=GtBuffer::_alive;
        qDebug()<< "use out buffer " <<b->getType() << b->objectName();
    }

    // проверяем сигналы в буфера на прием
    QList<BaseObject*> lo=GORKA->findChildren<BaseObject*>();
    foreach (BaseObject *b, lo) {
        // проверяем буфера
        for (int idx = 0; idx < b->metaObject()->propertyCount(); idx++) {
            QMetaProperty metaProperty = b->metaObject()->property(idx);
            int type = metaProperty.userType();
            if (type == qMetaTypeId<SignalDescription>()){
                QVariant val=metaProperty.read(b);
                SignalDescription p = val.value<SignalDescription>();
                if (p.isNotUse()) continue;
                if (p.isEmpty())continue;
                if (p.isInnerUse()) continue;
                if (l_out_buffers.indexOf(p.getBuffer())>=0) {
                    err(QString("%1->%2 out buffer use like inner %3").arg(b->objectName())
                        .arg(metaProperty.name())
                        .arg(p.getBuffer()->name));
                    noerr=false;
                }
            }
        }
    }
    return noerr;
}

void UVK_Central::work()
{
    QDateTime T=QDateTime::currentDateTime();

    GORKA->updateStates();

    // test
    if (testMode==1){
        int new_regim=testRegim();
        QString s;
        cmd_setRegim(new_regim,s);
    }

    TOS->work(T);
    GAC->work(T);
    otcepsController->work(T);
    sendBuffers();

    // РРС
    if ((!GORKA->SIGNAL_RRC_TU().isNotUse())&&(!GORKA->SIGNAL_RRC_TU().isEmpty())){
        int state=0;
        if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimStop) state=1;
        if (!GORKA->STATE_RRC()!=state){
            gac_command(GORKA->SIGNAL_RRC_TU(),state);
        }
    }




}

void UVK_Central::recv_cmd(QMap<QString, QString> m)
{
    // только свои
    if (m["DEST"]!="UVK") return;
    QString acceptStr="";
    qDebug()<< "CMD:"<< m["CMD"]+" ";
    if (m["CMD"]=="SET_RIGIME"){
        if (m["REGIME"]=="ROSPUSK"){
            if (cmd_setRegim(ModelGroupGorka::regimRospusk,acceptStr))
                CMD->accept_cmd(m,1,acceptStr); else
                CMD->accept_cmd(m,-1,acceptStr);
        }
        if (m["REGIME"]=="PAUSE"){
            if (cmd_setRegim(ModelGroupGorka::regimPausa,acceptStr))
                CMD->accept_cmd(m,1,acceptStr); else
                CMD->accept_cmd(m,-1,acceptStr);
        }
        if (m["REGIME"]=="STOP"){
            if (cmd_setRegim(ModelGroupGorka::regimStop,acceptStr))
                CMD->accept_cmd(m,1,acceptStr); else
                CMD->accept_cmd(m,-1,acceptStr);
        }
    }
    if (m["CMD"]=="SET_ACT_ZKR"){
        if (cmd_setPutNadvig(m["ACT_ZKR"].toInt(),acceptStr))
            CMD->accept_cmd(m,1,acceptStr); else
            CMD->accept_cmd(m,-1,acceptStr);
    }
    if (m["CMD"]=="OTCEPS"){

        if (m["CLEAR_ALL"]=="1"){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                otcepsController->cmd_CLEAR_ALL(acceptStr);
                TOS->resetTracking();
                CMD->accept_cmd(m,1,acceptStr);
                //                GAC->resetStates();
            } else {
                CMD->accept_cmd(m,-1,"Команда на очистку списка в режиме РОСПУСК.");
            }
        }
        if (m["ACTIVATE_ALL"]=="1"){
            if (GORKA->STATE_REGIM()==ModelGroupGorka::regimStop)
                otcepsController->cmd_ACTIVATE_ALL(acceptStr);
            TOS->resetTracking();
            CMD->accept_cmd(m,1,acceptStr);
            //                GAC->resetStates();
        }
        if (!m["DEL_OTCEP"].isEmpty()){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                if (otcepsController->cmd_DEL_OTCEP(m,acceptStr)){
                    int N=m["DEL_OTCEP"].toInt();
                    TOS->resetTracking(N);
                    CMD->accept_cmd(m,1,acceptStr);
                }else
                    CMD->accept_cmd(m,-1,acceptStr);
            }
        }
        if (!m["INC_OTCEP"].isEmpty()){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                if (otcepsController->cmd_INC_OTCEP(m,acceptStr))
                    CMD->accept_cmd(m,1,acceptStr); else
                    CMD->accept_cmd(m,-1,acceptStr);
            }
        }
    }
    if (m["CMD"]=="SET_OTCEP_STATE"){
        if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
            if (otcepsController->cmd_SET_OTCEP_STATE(m,acceptStr))
                CMD->accept_cmd(m,1,acceptStr); else
                CMD->accept_cmd(m,-1,acceptStr);
        }
    }
    if (m["CMD"]=="ADD_OTCEP_VAG"){
        if (otcepsController->cmd_ADD_OTCEP_VAG(m,acceptStr))
            CMD->accept_cmd(m,1,acceptStr); else
            CMD->accept_cmd(m,-1,acceptStr);
    }

    if (m["CMD"]=="RESET_DSO_BUSY"){
        if (TOS->resetDSOBUSY(m["RC"],acceptStr))
            CMD->accept_cmd(m,1,acceptStr); else
            CMD->accept_cmd(m,-1,acceptStr);
    }

    sendBuffers();
    qDebug()<< acceptStr;
}

void UVK_Central::gac_command(const SignalDescription &s, int state)
{
    if (testMode!=0) return;
    tu_cmd c;
    c.number=s.chanelOffset();
    c.on_off=state;
    do_message(&c).commit();
    udp->sendData(s.chanelType(),s.chanelName(),QByteArray((const char *)&c,sizeof(c)));
}



GtBuffer *oldestBuffer(QList<GtBuffer*> &l_buffers){
    if (l_buffers.isEmpty()) return nullptr;
    GtBuffer *oldb=l_buffers.first();
    foreach (GtBuffer *b, l_buffers) {
        if (!b->timeDataRecived.isValid()) return b;
        if (b->timeDataRecived>oldb->timeDataRecived) oldb=b;
    }
    return oldb;
}

void UVK_Central::sendBuffers()
{
    QDateTime T=QDateTime::currentDateTime();
    // сохранили старое значение
    foreach (GtBuffer*B, l_out_buffers) {
        mB2A[B]=B->A;
    }
    // записали новое
    state2buffer();

    foreach (GtBuffer*B, l_out_buffers) {
        if (mB2A[B]!=B->A){
            B->timeDataChanged=T;
            B->timeDataRecived=T;
            udp->sendGtBuffer(B);
        };
    }
    foreach (GtBuffer*B, l_out_buffers) {
        int period=1000;
        if ((B->getType()==9)|| ((B->getType()==109))) period=10000;
        if ((B->getType()==15)) period=20000;
        if ( B->timeDataRecived.msecsTo(T)>period){
            B->timeDataRecived=T;
            udp->sendGtBuffer(B);

        };
    }
    GtBuffer*B=oldestBuffer(l_out_buffers);
    if (B!=nullptr) {
        if ((!B->timeDataRecived.isValid())||(B->timeDataRecived.msecsTo(T)>20)){
            udp->sendGtBuffer(B);
            B->timeDataRecived=T;
        }

    }

}

int UVK_Central::getNewOtcep(m_RC*rc)
{
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk){
        // проверим что едем с нужной зкр
        foreach (auto zkr, l_zkr) {
            if ((rc==zkr) && (zkr->STATE_ROSPUSK()==1)){
                m_Otcep *otcep=otcepsController->otceps->topOtcep();
                if (otcep!=nullptr)  {
                    if (otcep->NUM()>maxOtcepCurrenRospusk) maxOtcepCurrenRospusk=otcep->NUM();
                    return otcep->NUM();
                }
                otcep=otcepsController->otceps->otcepByNum(maxOtcepCurrenRospusk+1);
                if (otcep!=nullptr)  {
                    otcep->resetStates();
                    otcep->setSTATE_ENABLED(true);
                    if (otcep->NUM()>maxOtcepCurrenRospusk) maxOtcepCurrenRospusk=otcep->NUM();
                    return otcep->NUM();
                }



            }
        }

    }
    return 0;
}

QList<SignalDescription> UVK_Central::acceptOutputSignals()
{
    GORKA->setSIGNAL_ROSPUSK(GORKA->SIGNAL_ROSPUSK().innerUse());
    GORKA->setSIGNAL_PAUSA(GORKA->SIGNAL_PAUSA().innerUse());
    GORKA->setSIGNAL_STOP(GORKA->SIGNAL_STOP().innerUse());

    QList<SignalDescription> l;
    l << GORKA->SIGNAL_ROSPUSK()
      << GORKA->SIGNAL_PAUSA()
      << GORKA->SIGNAL_STOP();
    foreach (auto zkr, l_zkr) {
        zkr->setSIGNAL_ROSPUSK(zkr->SIGNAL_ROSPUSK().innerUse());
        l<<zkr->SIGNAL_ROSPUSK();
    }
    return l;
}


void UVK_Central::state2buffer()
{
    GORKA->SIGNAL_ROSPUSK().setValue_1bit(0);
    GORKA->SIGNAL_PAUSA().setValue_1bit(0);
    GORKA->SIGNAL_STOP().setValue_1bit(0);
    switch (GORKA->STATE_REGIM()) {
    case ModelGroupGorka::regimRospusk: GORKA->SIGNAL_ROSPUSK().setValue_1bit(1); break;
    case ModelGroupGorka::regimPausa: GORKA->SIGNAL_PAUSA().setValue_1bit(1); break;
    case ModelGroupGorka::regimStop: GORKA->SIGNAL_STOP().setValue_1bit(1); break;
    }
    // выставляем напрямую в модели
    foreach (auto zkr, l_zkr) {
        zkr->SIGNAL_ROSPUSK().setValue_1bit(zkr->STATE_ROSPUSK());
    }

    TOS->state2buffer();
    GAC->state2buffer();
    otcepsController->state2buffer();

}
bool UVK_Central::cmd_setPutNadvig(int p,QString &acceptStr)
{
    bool ex=false;
    foreach (auto rc_zkr, TOS->l_zkr) {
        if (rc_zkr->PUT_NADVIG()==p){
            rc_zkr->setSTATE_ROSPUSK(true);
            ex=true;
        } else {
            rc_zkr->setSTATE_ROSPUSK(false);
        }
    }
    if (ex) {
        acceptStr=QString("Путь надвига %1.").arg(p);
        return true;
    }
    acceptStr=QString("Путь надвига %1. не установлен").arg(p);
    return false;
}

bool UVK_Central::cmd_setRegim(int p,QString &acceptStr)
{
    switch (GORKA->STATE_REGIM()) {
    case ModelGroupGorka::regimUnknow:
        GORKA->setSTATE_REGIM(p);
        acceptStr=QString("Режим %1 установлен.").arg(p);
        return true;
    case ModelGroupGorka::regimRospusk:
    {
        switch (p) {
        case ModelGroupGorka::regimRospusk:
            acceptStr=QString("Режим РОСПУСК уже установлен.");
            return true;
        case ModelGroupGorka::regimPausa:
            // выявления нет
            acceptStr=QString("Режим ПАУЗА установлен.");
            GORKA->setSTATE_REGIM(p);
            return true;
        case ModelGroupGorka::regimStop:
            // продолжаем следить
            // стрелк не переводим
            acceptStr=QString("Режим КОНЕЦ РОСПУСКА установлен.");
            GORKA->setSTATE_REGIM(p);
            GAC->resetStates();
            GAC->setSTATE_ENABLED(false);
            return true;
        }
    } break;
    case ModelGroupGorka::regimPausa:
    {
        switch (p) {
        case ModelGroupGorka::regimRospusk:
            acceptStr=QString("Режим РОСПУСК установлен.");
            GORKA->setSTATE_REGIM(p);
            return true;
        case ModelGroupGorka::regimPausa:
            acceptStr=QString("Режим ПАУЗА уже установлен.");
            return true;
        case ModelGroupGorka::regimStop:
            acceptStr=QString("Режим КОНЕЦ РОСПУСКА установлен.");
            GORKA->setSTATE_REGIM(p);
            GAC->resetStates();
            GAC->setSTATE_ENABLED(false);
            return true;
        }
    } break;
    case ModelGroupGorka::regimStop:
    {
        switch (p) {
        case ModelGroupGorka::regimRospusk:
            acceptStr=QString("Режим РОСПУСК установлен.");
            maxOtcepCurrenRospusk=0;
            TOS->resetTracking();
            GAC->resetStates();
            TOS->setSTATE_ENABLED(true);
            GAC->setSTATE_ENABLED(true);
            GORKA->setSTATE_REGIM(p);
            return true;
        case ModelGroupGorka::regimPausa:
            acceptStr="Нет перехода режим СТОП -> ПАУЗА";
            return false;
        case ModelGroupGorka::regimStop:
            acceptStr=QString("Режим КОНЕЦ РОСПУСКА установлен.");
            GORKA->setSTATE_REGIM(p);
            GAC->resetStates();
            GAC->setSTATE_ENABLED(false);
            return true;
        }
    } break;
    }
    acceptStr=QString("Режим %1 не установлен.").arg(p);
    return false;
}





int UVK_Central::testRegim()
{
    auto zkr=GORKA->active_zkr();
    if ((zkr!=nullptr)&&(zkr->svet()!=nullptr)){
        if  (zkr->svet()->STATE_Z()==1) return ModelGroupGorka::regimRospusk;
    }
    return ModelGroupGorka::regimStop;
}
