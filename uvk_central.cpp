#include "uvk_central.h"
#include <QFileInfo>

UVK_Central::UVK_Central(QObject *parent) : QObject(parent)
{

}

bool UVK_Central::init(QString fileNameModel)
{
    if (!QFileInfo(fileNameModel).exists()){
        qDebug() << "file not found " << QFileInfo(fileNameModel).absoluteFilePath();
        return false;
    }
    udp=new GtBuffers_UDP_D2();
    MVP.setGetGtBufferInterface(udp);
    if (fileNameModel.isEmpty()) fileNameModel="./M.xml";
    QObject *O=MVP.loadObject(fileNameModel);
    GORKA=qobject_cast<ModelGroupGorka *>(O);
    if (!GORKA){
        err(QString("Нет объекта ModelGroupGorka в %1").arg(fileNameModel));
        return false;
    }
    qDebug() << "model load:" << QFileInfo(fileNameModel).absoluteFilePath();

    TOS=new TrackingOtcepSystem(this,GORKA);

    GAC=new GtGac(this,TOS);

    CMD=new GtCommandInterface(this,udp);

    acceptSignals();
    acceptBuffers();


    return validation();


}

void UVK_Central::start()
{
    connect(CMD,&GtCommandInterface::recv_cmd,this,&UVK_Central::recv_cmd);
    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,&UVK_Central::work);
    timer->start(10);
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
                if (l_buffers.indexOf(p.getBuffer())>=0) {
                    err(QString("%1->%2 use inner buffer %3").arg(b->objectName())
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
                    if (l_buffers.indexOf(p.getBuffer())<0) {
                        l_buffers.push_back(p.getBuffer());
                        p.getBuffer()->static_mode=true;
                        qDebug()<< "accept inner buffer " <<p.getBuffer()->name;
                    }
                }
            }
        }
    }
}

void UVK_Central::work()
{
    QDateTime T=QDateTime::currentDateTime();
    GORKA->updateStates();
    TOS->work(T);
    GAC->work(T);
    sendBuffers();
}

void UVK_Central::recv_cmd(QMap<QString, QString> m)
{
    // только свои
    if (m["DEST"]!="UVK") return;
    if (m["CMD"]=="SET_RIGIME"){
        if (m["REGIME"]=="ROSPUSK"){
            setRegim(ModelGroupGorka::regimRospusk);
        }
        if (m["REGIME"]=="PAUSE"){
            setRegim(ModelGroupGorka::regimPausa);
        }
        if (m["REGIME"]=="STOP"){
            setRegim(ModelGroupGorka::regimStop);
        }
    }
    if (m["CMD"]=="SET_ACT_ZKR"){
        if (m["ACT_ZKR"]=="1"){
            setPutNadvig(1);
        }
        if (m["ACT_ZKR"]=="2"){
            setPutNadvig(2);
        }
    }
    if (m["CMD"]=="OTCEPS"){

        if (m["CLEAR_ALL"]=="1"){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                TOS->otceps->resetStates();
                TOS->resetStates();
                GAC->resetStates();
            }
            if (m["ACTIVATE_ALL"]=="1"){
                if (GORKA->STATE_REGIM()==ModelGroupGorka::regimStop)
                    foreach (m_Otcep*otcep, TOS->lo) {
                        if (otcep->STATE_MAR()>0) otcep->setSTATE_ENABLED(true);
                    }
                TOS->resetStates();
                GAC->resetStates();
            }
            if (!m["DEL_OTCEP"].isEmpty()){
                if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                    int N=m["DEL_OTCEP"].toInt();
                    if ((N>0)&& (N<TOS->lo.size())){
                        for (int i=N-1;i<TOS->lo.size()-1;i++){
                            TOS->lo[i]->acceptStaticData(TOS->lo[i+1]);
                        }
                        TOS->lo.last()->resetStates();
                    }
                }
            }
            if (!m["INC_OTCEP"].isEmpty()){
                if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                    int N=m["INC_OTCEP"].toInt();
                    if ((N>0)&& (N<TOS->lo.size()-1)){
                        for (int i=TOS->lo.size()-1;i>=N-1+1;i--){
                            TOS->lo[i]->acceptStaticData(TOS->lo[i-1]);
                        }
                        TOS->lo[N-1]->resetStates();

                    }
                }
            }

        }
    }
    if (m["CMD"]=="SET_OTCEP_STATE"){
        setOtcepStates(m);

    }
}

void UVK_Central::sendBuffers()
{
    QDateTime T=QDateTime::currentDateTime();
    foreach (GtBuffer*B, l_buffers) {
        mB2A[B]=B->A;
    }
    this->state2buffer();
    TOS->state2buffer();
    GAC->state2buffer();
    foreach (GtBuffer*B, l_buffers) {
        if (mB2A[B]!=B->A){
            B->timeDataChanged=T;
            B->timeDataRecived=T;
            udp->sendGtBuffer(B);
        };
    }
    foreach (GtBuffer*B, l_buffers) {
        if ( B->timeDataRecived.msecsTo(T)>1000){
            B->timeDataRecived=T;
            udp->sendGtBuffer(B);
        };
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
        if (zkrt->rc_zkr->PUTNADVIG()==p){
            zkrt->rc_zkr->setSTATE_ROSPUSK(true);
        } else {
            zkrt->rc_zkr->setSTATE_ROSPUSK(false);
        }

    }
}

void UVK_Central::setRegim(int p)
{
    if (p==ModelGroupGorka::regimStop){
        GORKA->setSTATE_REGIM(ModelGroupGorka::regimStop);
        TOS->setSTATE_ENABLED(false);
        GAC->setSTATE_ENABLED(false);
        return;
    }
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk){
        if (p==ModelGroupGorka::regimPausa){
            GORKA->setSTATE_REGIM(ModelGroupGorka::regimPausa);
            TOS->setSTATE_ENABLED(true);
            GAC->setSTATE_ENABLED(false);
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

void UVK_Central::setOtcepStates(QMap<QString, QString> &m)
{
    if (!m["N"].isEmpty()){
        if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
            int N=m["N"].toInt();
            if ((N>0)&& (N<TOS->lo.size()-1)){
                m_Otcep *otcep=TOS->lo[N-1];
                foreach (QString key, m.keys()) {
                    if ((key=="CMD") || (key=="N")) continue;
                    QString stateName="STATE_"+key;
                    QVariant V;
                    V=m[key];

                    for (int idx = 0; idx < otcep->metaObject()->propertyCount(); idx++) {
                        QMetaProperty metaProperty = otcep->metaObject()->property(idx);
                        if (metaProperty.name()!=stateName) continue;
                        metaProperty.write(otcep,V);

                    }
                }
            }
        }
    }
}


