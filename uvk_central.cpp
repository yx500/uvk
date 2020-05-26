#include "uvk_central.h"
#include <QFileInfo>
#include "do_message.hpp"

UVK_Central::UVK_Central(QObject *parent) : QObject(parent)
{

}

bool UVK_Central::init(QString fileNameModel)
{


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

    otcepsController=new OtcepsController(this,l_otceps.first());
    otcepsController->disableBuffers();
    TOS=new TrackingOtcepSystem(this,GORKA);
    TOS->disableBuffers();

    GAC=new GtGac(this,TOS);

    CMD=new GtCommandInterface(this,udp);
    CMD->setSRC_ID("UVK");

    acceptSignals();
    acceptBuffers();

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
    setPutNadvig(1);
    setRegim(0);
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
    return validateBuffers();
}

bool UVK_Central::validateBuffers()
{
    bool noerr=true;
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

void UVK_Central::acceptBuffers()
{
    // закрываем буфера на прием
    QList<BaseObject*> lo=GORKA->findChildren<BaseObject*>();

    foreach (BaseObject *b, lo) {
        // собираем буфера
        for (int idx = 0; idx < b->metaObject()->propertyCount(); idx++) {
            QMetaProperty metaProperty = b->metaObject()->property(idx);
            int type = metaProperty.userType();
            if (type == qMetaTypeId<SignalDescription>()){
                QVariant val=metaProperty.read(b);
                SignalDescription p = val.value<SignalDescription>();
                if (p.isEmpty())continue;
                if (p.isInnerUse()) {
                    if (l_out_buffers.indexOf(p.getBuffer())<0) {
                        l_out_buffers.push_back(p.getBuffer());
                        p.getBuffer()->static_mode=true;
                    }
                }
            }
        }
    }
    for (int i=0;i<MaxVagon;i++){
        if (TOS->otceps->chanelVag[i]->static_mode==true)
        {
            if (l_out_buffers.indexOf(TOS->otceps->chanelVag[i])<0)
                l_out_buffers.push_back(TOS->otceps->chanelVag[i]);
        }
    }
    foreach (auto b, udp->allBuffers()) {
        if (!b->static_mode)
            qDebug()<< "use in buffer " <<b->getType() << b->objectName();
    }
    foreach (auto b, l_out_buffers) {
        qDebug()<< "use out buffer " <<b->getType() << b->objectName();
    }
}

void UVK_Central::work()
{
    QDateTime T=QDateTime::currentDateTime();
    GORKA->updateStates();
    TOS->work(T);
    GAC->work(T);
    otcepsController->work(T);
    sendBuffers();
}

void UVK_Central::recv_cmd(QMap<QString, QString> m)
{
    // только свои
    if (m["DEST"]!="UVK") return;
    QString dbgStr="CMD:"+m["CMD"]+" ";
    if (m["CMD"]=="SET_RIGIME"){
        if (m["REGIME"]=="ROSPUSK"){
            setRegim(ModelGroupGorka::regimRospusk);
            dbgStr+="ROSPUSK accepted";
        }
        if (m["REGIME"]=="PAUSE"){
            setRegim(ModelGroupGorka::regimPausa);
            dbgStr+="PAUSE accepted";
        }
        if (m["REGIME"]=="STOP"){
            setRegim(ModelGroupGorka::regimStop);
            dbgStr+="STOP accepted";
        }
    }
    if (m["CMD"]=="SET_ACT_ZKR"){
        if (m["ACT_ZKR"]=="1"){
            setPutNadvig(1);
            dbgStr+="1 accepted";
        }
        if (m["ACT_ZKR"]=="2"){
            setPutNadvig(2);
            dbgStr+="2 accepted";
        }
    }
    if (m["CMD"]=="OTCEPS"){

        if (m["CLEAR_ALL"]=="1"){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                otcepsController->cmd_CLEAR_ALL(m);
                TOS->resetStates();
                //                GAC->resetStates();
                dbgStr+="CLEAR_ALL accepted";
            }
            if (m["ACTIVATE_ALL"]=="1"){
                if (GORKA->STATE_REGIM()==ModelGroupGorka::regimStop)
                    otcepsController->cmd_ACTIVATE_ALL(m);
                TOS->resetStates();
                //                GAC->resetStates();
                dbgStr+="ACTIVATE_ALL accepted";
            }
            if (!m["DEL_OTCEP"].isEmpty()){
                if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                    otcepsController->cmd_DEL_OTCEP(m);
                    dbgStr+="DEL_OTCEP accepted";
                }
            }
        }
        if (!m["INC_OTCEP"].isEmpty()){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                otcepsController->cmd_INC_OTCEP(m);

                dbgStr+="INC_OTCEP accepted";

            }

        }
    }
    if (m["CMD"]=="SET_OTCEP_STATE"){
        if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
            otcepsController->cmd_SET_OTCEP_STATE(m);
            dbgStr+="accepted";
        }
    }
    if (m["CMD"]=="ADD_OTCEP_VAG"){
        if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk) {
            if (otcepsController->cmd_ADD_OTCEP_VAG(m))
                dbgStr+="accepted";
        }
    }
    sendBuffers();
    qDebug()<< dbgStr;
}

void UVK_Central::gac_command(const SignalDescription &s, int state)
{
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
    foreach (GtBuffer*B, l_out_buffers) {
        mB2A[B]=B->A;
    }
    this->state2buffer();
    TOS->state2buffer();
    GAC->state2buffer();
    otcepsController->state2buffer();
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

void UVK_Central::acceptSignals()
{
    GORKA->setSIGNAL_ROSPUSK(GORKA->SIGNAL_ROSPUSK().innerUse());
    GORKA->setSIGNAL_PAUSA(GORKA->SIGNAL_PAUSA().innerUse());
    GORKA->setSIGNAL_STOP(GORKA->SIGNAL_STOP().innerUse());
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


}
void UVK_Central::setPutNadvig(int p)
{

    foreach (auto zkrt, TOS->l_zkrt) {
        if (zkrt->rc_zkr->PUT_NADVIG()==p){
            zkrt->rc_zkr->setSTATE_ROSPUSK(true);
        } else {
            zkrt->rc_zkr->setSTATE_ROSPUSK(false);
        }

    }
}

void UVK_Central::setRegim(int p)
{
    if (p==ModelGroupGorka::regimStop){
        GORKA->setSTATE_REGIM(p);
        // продолжаем следить
//        foreach (auto otcep, otcepsController->otceps->l_otceps) {
//            otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
//        }
        //TOS->setSTATE_ENABLED(false);
        GAC->resetStates();
        GAC->setSTATE_ENABLED(false);
        return;
    }
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk){
        if (p==ModelGroupGorka::regimPausa){
            GORKA->setSTATE_REGIM(ModelGroupGorka::regimPausa);
            // продолжаем следить и управлять стрелками
            // выявления нет
//            TOS->setSTATE_ENABLED(true);
//            GAC->resetStates();
//            GAC->setSTATE_ENABLED(false);
        }
    }
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimPausa){
        if (p==ModelGroupGorka::regimRospusk){
            GORKA->setSTATE_REGIM(ModelGroupGorka::regimRospusk);
            TOS->setSTATE_ENABLED(true);
            GAC->setSTATE_ENABLED(true);
        }

    }
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimStop){
        if (p==ModelGroupGorka::regimRospusk){
            GORKA->setSTATE_REGIM(ModelGroupGorka::regimRospusk);
            TOS->resetStates();
            GAC->resetStates();
            TOS->setSTATE_ENABLED(true);
            GAC->setSTATE_ENABLED(true);
        }

    }
}





