#include "uvk_central.h"
#include <QFileInfo>
#include "messageDO.h"
#include "tos_system_dso.h"
#include "gtapp_watchdog.h"
#include "tStatPc.h"

int testMode=0;
int testMode_no_send=0;
int testMode_no_check_alive=0;

static gtapp_watchdog dog("dd");
class IUVKGetNewOtcep :public IGetNewOtcep
{
public:
    IUVKGetNewOtcep(UVK_Central *uvk){this->uvk=uvk;}
    virtual int getNewOtcep(m_RC*rc,int drobl){return uvk->getNewOtcep(rc,drobl);}
    virtual int resetOtcep2prib(int num){return uvk->resetOtcep2prib(num);}
    virtual int nerascep(int num){return uvk->nerascep(num);}
    UVK_Central *uvk;
};


UVK_Central::UVK_Central(QObject *parent) : QObject(parent)
{

}

bool UVK_Central::init(QString fileNameIni)
{
    if (QFileInfo::exists(fileNameIni)){
        qDebug() << "ini load:" << QFileInfo(fileNameIni).absoluteFilePath();
        {
            QSettings settings(fileNameIni,QSettings::IniFormat);
            fileNameModel=settings.value("main/model","./M.xml").toString();
            signal_SlaveMode.fromString(settings.value("main/signal_SlaveMode","").toString());
            signal_Control.fromString(settings.value("main/signal_Control","").toString());
            uvkStatusName=settings.value("main/status","").toString();

            testMode=settings.value("test/mode",0).toInt();
            testMode_no_send=settings.value("test/no_send",0).toInt();
            testMode_no_check_alive=settings.value("test/no_check_alive",0).toInt();
            watchdog=new gtapp_watchdog(qPrintable(settings.value("main/watchdog","uvk").toString()));
            settings.beginGroup("shared_memory");
            sl_memshared_buffers= settings.childKeys();


        }
    } else {
        {
            QSettings settings(fileNameIni,QSettings::IniFormat);
            settings.setValue("main/model","./M.xml");
            settings.setValue("main/signal_SlaveMode",SignalDescription(1,"name_buf_ts",777).toString());
            settings.setValue("main/signal_Control",SignalDescription(1,"name_buf_tu",777).toString());
            settings.setValue("main/status","uvk_st");
            settings.setValue("main/watchdog","uvk");
            fileNameIni=settings.fileName();
            settings.setValue("test/mode","0");
            settings.setValue("test/no_send","0");
            settings.setValue("test/no_check_alive","0");
            settings.setValue("shared_memory/name_of_ts_buffer","1");

        }
        qDebug() << "ini created:" << QFileInfo(fileNameIni).absoluteFilePath();
        return false;
    }


    if (!QFileInfo::exists(fileNameModel)){
        qFatal("file not found %s",QFileInfo(fileNameModel).absoluteFilePath().toStdString().c_str());
        return false;
    }
    udp=new GtBuffers_UDP_D2();
    MVP.setGetGtBufferInterface(udp);
    if (testMode_no_send==1) udp->no_send=true;

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
    if (!signal_SlaveMode.isEmpty()) signal_SlaveMode.acceptGtBuffer();
    //    udp->getGtBuffer(3,"uvk_status")->static_mode=true;

    // парамеры ГАЦ
    foreach (auto gstr, GAC->l_strel) {
        ilog(QString("%1+ Vгран=%2 НГБ РЦ=%3").arg(gstr->strel->objectName()).arg(gstr->strel->NEGAB_VGRAN_P()).arg(gstr->strel->l_ngb_rc[0].size()));
        foreach (auto _rc, gstr->strel->l_ngb_rc[0]) {
            ilog(QString("%1+ НГБ РЦ %2 LEN=%3").arg(gstr->strel->objectName()).arg(_rc->objectName()).arg(_rc->LEN()));
        }
        ilog(QString("%1- Vгран=%2 НГБ РЦ=%3").arg(gstr->strel->objectName()).arg(gstr->strel->NEGAB_VGRAN_M()).arg(gstr->strel->l_ngb_rc[1].size()));
        foreach (auto _rc, gstr->strel->l_ngb_rc[1]) {
            ilog(QString("%1- НГБ РЦ %2 LEN=%3").arg(gstr->strel->objectName()).arg(_rc->objectName()).arg(_rc->LEN()));
        }

    }



    GORKA->resetStates();
    TOS->resetStates();
    TOS->resetTracking();
    GAC->resetStates();
    otcepsController->resetStates();

    qDebug() << "status:" << uvkStatusName;
    qDebug() << "signal_SlaveMode:" << signal_SlaveMode.toString();
    qDebug() << "signal_Control:" << signal_Control.toString();
    if (testMode!=0) qDebug() << "testMode:" << testMode;

    return validation();

}

void UVK_Central::start()
{
    connect(GAC,&GtGac::uvk_command,this,&UVK_Central::gac_command);

    connect(CMD,&GtCommandInterface::recv_cmd,this,&UVK_Central::recv_cmd);

    timer_work=new QTimer(this);
    connect(timer_work,&QTimer::timeout,this,&UVK_Central::work);

    timer_send=new QTimer(this);
    connect(timer_send,&QTimer::timeout,this,&UVK_Central::sendBuffersPeriod);

    auto timer_status=new QTimer(this);
    connect(timer_status,&QTimer::timeout,this,&UVK_Central::sendStatus);


    // перебрасываем все каналы на себя
    // только 1 тип
    foreach (GtBuffer *B, udp->allBuffers()) {
        if ((B->type==1)&&(!B->static_mode)){
            connect(B,&GtBuffer::bufferChanged,this,&UVK_Central::work);
            qDebug()<< "active buffer" << B->type << " "<<B->name;
        }
    }
    maxOtcepCurrenRospusk=0;
    QString tmpstr;
    cmd_setPutNadvig(1,tmpstr);
    cmd_setRegim(0,tmpstr);
    // инициировали
    foreach (GtBuffer*B, l_out_buffers) {
        mB2A[B]=B->A;
    }

    sendBuffers(_all);

    start_time=QDateTime::currentDateTimeUtc().toTime_t();

    timer_work->start(100);
    timer_send->start(50);
    timer_status->start(500);

    qDebug() << "uvk started ";
}

void UVK_Central::err( QString errstr)
{

    errLog.push_back(errstr);
}

void UVK_Central::ilog(QString str)
{
    initLog.push_back(str);
}

bool UVK_Central::validation()
{
    ListObjStr l;
    l.clear();
    GORKA->validation(&l);
    ilog("Validation:");
    initLog.append(l.toStringList());
    if (l.contains_error){
        err("GORKA errors:");
        errLog.append(l.toStringList());
        return false;
    }
    l.clear();
    TOS->validation(&l);
    initLog.append(l.toStringList());
    if (l.contains_error){
        err("TOS errors:");
        errLog.append(l.toStringList());
        return false;
    }
    l.clear();
    GAC->validation(&l);
    initLog.append(l.toStringList());
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



    QList<GtBuffer*> l_tu_buffers;

    // закрываем буфера на прием
    foreach (auto p, l) {
        if (p.isEmpty())continue;
        auto B=p.getBuffer();
        if (l_out_buffers.indexOf(p.getBuffer())<0) {

            // ставим период рассылки
            switch (B->type){
            case 0:B->setMsecPeriodLive(0); break;
            case 1:B->setMsecPeriodLive(1000);B->setSizeData(121); break;
            case 9:B->setMsecPeriodLive(20000); break;
            case 15:B->setMsecPeriodLive(30000); break;
            case 17:B->setMsecPeriodLive(1000); break;
            default:B->setMsecPeriodLive(1000);
            }
            if (B->type==0){
                if (!l_tu_buffers.contains(B)){
                    l_tu_buffers << B;
                }
            } else {
                l_out_buffers.push_back(p.getBuffer());
            }
            B->static_mode=true;
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


    foreach (auto b, udp->allBuffers()) {
        if (!b->static_mode){
            ilog(QString("use in buffer %1 %2").arg(b->getType()).arg(b->objectName()));
            // ставим период ТО
            switch (b->type){
            case 1:b->setMsecPeriodLive(2000);b->setSizeData(121); break;
            }
            if (testMode_no_check_alive==1){
                switch (b->type){
                case 1:b->setMsecPeriodLive(0);break;
                }
            }
        }
    }
    foreach (auto b, l_out_buffers) {
        b->sost=GtBuffer::_alive;
        ilog(QString("use out buffer %1 %2").arg(b->getType()).arg(b->objectName()));
    }
    foreach (auto b, l_tu_buffers) {
        ilog(QString("use tu buffer %1 %2").arg(b->getType()).arg(b->objectName()));
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

    // запихиваем в shared
    foreach (auto buf_name, sl_memshared_buffers) {
        auto buf=udp->getBufferEx(1,buf_name);
        if (buf==nullptr) {
            err(QString("no buffer in model to shared: %1").arg(buf_name));
            noerr=false;
        } else {

            GtNetSharedMem *gtNetSB =new GtNetSharedMem(nullptr,buf);
            if (!gtNetSB->mem->isAttached()){
                err(QString("no buffer in memory for shared: %1").arg(buf_name));
                delete gtNetSB;
                noerr=false;
            } else {
                buf->shared_mem=true;
                connect(gtNetSB,&GtNetSharedMem::changeBuffer,udp,&GtBuffers_UDP_D2::bufferChanged);
                gtNetSB->start();
                gtNetSB->setPriority(QThread::HighPriority);
                ilog(QString("use shared mem buffer %1 %2").arg(buf->getType()).arg(buf->objectName()));
            }

        }

    }

    return noerr;
}

void UVK_Central::work()
{
    watchdog->gav();
    QDateTime T=QDateTime::currentDateTime();

    // определяем свой статус
    slaveMode=false;
    if (!signal_SlaveMode.isEmpty()){
        if (signal_SlaveMode.value_1bit()==0){
            slaveMode=true;
        }
    }
    if (udp->slaveMode!=slaveMode){
        udp->slaveMode=slaveMode;
        qDebug()<< "slaveMode=" <<slaveMode;
    }

    GORKA->updateStates();
    //    QDateTime T1s=QDateTime::currentDateTime();
    // test
    if (testMode==1){
        int new_regim=testRegim();
        QString s;
        if ((GORKA->STATE_REGIM()==ModelGroupGorka::regimStop) && (new_regim==ModelGroupGorka::regimRospusk)) {
            foreach (auto otcep, otcepsController->otceps->otceps()) {
                if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib){
                    otcep->resetStates();
                }
            }
        }
        if (GORKA->STATE_REGIM()!=new_regim)
            cmd_setRegim(new_regim,s);
    }

    TOS->work(T);
    GAC->work(T);
    otcepsController->work(T);
    checkFinishRospusk(T);
    //    QDateTime T2w=QDateTime::currentDateTime();

    // РРС
    static QElapsedTimer rrc_emit;
    if ((!GORKA->SIGNAL_RRC_TU().isNotUse())&&(!GORKA->SIGNAL_RRC_TU().isEmpty())){
        int state=0;
        if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimStop) state=1;
        if (GORKA->STATE_RRC()!=state){
            if ((!rrc_emit.isValid())||(rrc_emit.elapsed()>1000)){
                gac_command(GORKA->SIGNAL_RRC_TU(),state);
                rrc_emit.restart();
            }

        }
    }
    if (udp->get_emit_counter()<=1){
        state2buffer();
        sendBuffers(_changed);
    } else{
        qDebug()<< "udp->emit_counter=" <<udp->get_emit_counter();
    }

    //    QDateTime T3sb=QDateTime::currentDateTime();


    //    QDateTime T4sn=QDateTime::currentDateTime();



    //    qDebug()<<
    //               "u="<<T1s.msecsTo(T) <<
    //               "w="<<T2w.msecsTo(T1s)<<
    //               "sb="<<T3sb.msecsTo(T2w)<<
    //               "u="<<T4sn.msecsTo(T3sb)
    //               ;

    //    QDateTime T4sn=QDateTime::currentDateTime();
    //    qDebug()<<"w="<<T4sn.msecsTo(T) ;
    watchdog->gav();

}

void UVK_Central::sendBuffersPeriod()
{
    sendBuffers(_period);
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
        if (m["UPDATE_ALL"]=="1"){
            otcepsController->cmd_UPDATE_ALL(acceptStr);
            CMD->accept_cmd(m,1,acceptStr);
        }
        if (!m["DEL_OTCEP"].isEmpty()){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                if (otcepsController->cmd_DEL_OTCEP(m,acceptStr)){
                    //                    int N=m["DEL_OTCEP"].toInt();
                    //TOS->resetTracking(N);
                    CMD->accept_cmd(m,1,acceptStr);
                }else
                    CMD->accept_cmd(m,-1,acceptStr);
            } else {
                CMD->accept_cmd(m,-1,"Попытка удалить отцеп в режиме РОСПУСК");
            }
        }
        if (!m["INC_OTCEP"].isEmpty()){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                if (otcepsController->cmd_INC_OTCEP(m,acceptStr))
                    CMD->accept_cmd(m,1,acceptStr); else
                    CMD->accept_cmd(m,-1,acceptStr);
            } else {
                CMD->accept_cmd(m,-1,"Попытка добавить отцеп в режиме РОСПУСК");

            }
        }
        if (!m["SET_CUR_OTCEP"].isEmpty()){
            if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
                if (otcepsController->cmd_SET_CUR_OTCEP(m,acceptStr))
                    CMD->accept_cmd(m,1,acceptStr); else
                    CMD->accept_cmd(m,-1,acceptStr);
            } else {
                CMD->accept_cmd(m,-1,"Попытка добавить отцеп в режиме РОСПУСК");

            }
        }
    }
    if (m["CMD"]=="SET_OTCEP_STATE"){
        //if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
        if (otcepsController->cmd_SET_OTCEP_STATE(m,acceptStr))
            CMD->accept_cmd(m,1,acceptStr); else
            CMD->accept_cmd(m,-1,acceptStr);
        //}
    }
    if (m["CMD"]=="ADD_OTCEP_VAG"){
        if (otcepsController->cmd_ADD_OTCEP_VAG(m,acceptStr))
            CMD->accept_cmd(m,1,acceptStr); else
            CMD->accept_cmd(m,-1,acceptStr);
    }

    if (m["CMD"]=="RESET_DSO_BUSY"){
        if (GORKA->STATE_REGIM()!=ModelGroupGorka::regimRospusk){
            if (TOS->resetDSOBUSY(m["RC"],acceptStr))
                CMD->accept_cmd(m,1,acceptStr); else
                CMD->accept_cmd(m,-1,acceptStr);
        } else {
            CMD->accept_cmd(m,-1,"Попытка снять занятость в режиме РОСПУСК");
        }
    }
    state2buffer();
    sendBuffers(_changed);
    qDebug()<< acceptStr;
}

void UVK_Central::gac_command(const SignalDescription &s, int state)
{
    if (testMode!=0) return;
    if (slaveMode!=0) return;
    message_DO c;
    //    memset(&c,0,sizeof(c));
    c.flag=0;
    c.number=s.chanelOffset();
    c.data[0]=state;
    c.commit();
    //    qDebug() << "gac_command" << c.is_valid();
    udp->sendData(s.chanelType(),s.chanelName(),QByteArray((const char *)&c,sizeof(c)));
}

void UVK_Central::sendStatus()
{
    struct UVK_Status{

        quint8 slaveMode;
        time_t start_time;

    };
    static UVK_Status c;
    c.start_time=start_time;
    c.slaveMode=slaveMode;

    udp->sendData(3,uvkStatusName,QByteArray((const char *)&c,sizeof(c)));

    // контролим свой статус
    if (!signal_Control.isEmpty()){
        cmd_DO c;
        c.flag=0;
        c.number=signal_Control.chanelOffset();
        c.data[0]=1;
        message_DO(&c).commit();
        udp->sendData(signal_Control.chanelType(),signal_Control.chanelName(),QByteArray((const char *)&c,sizeof(c)));
    }

    // шлем древний статус
    static QString old_status_name="uvk_st";
    tStatPC MyStat;
    memset(&MyStat,0,sizeof(tStatPC));
    MyStat.SostGen = cEndR;
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk) MyStat.SostGen = cBegiR;
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimPausa) MyStat.SostGen = cPausR;
    MyStat.TestCP = 0;//num_train;
    MyStat.TestLP = 0;//regim+1;
    MyStat.SostGenCPX = 0;//num_rosp;
    udp->sendData(3,old_status_name,QByteArray((const char *)&MyStat,sizeof(MyStat)));

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

void UVK_Central::sendBuffers(int p)
{
    QDateTime T=QDateTime::currentDateTime();

    l_buffers4send.clear();
    if (p==_changed){
        // записали новое
        foreach (GtBuffer*B, l_out_buffers) {
            if (mB2A[B]!=B->A){
                B->timeDataChanged=T;
                l_buffers4send.push_back(B);
            };
        }
    }
    if (p==_all){
        foreach (GtBuffer*B, l_out_buffers) {
            l_buffers4send.push_back(B);
        }
    }
    if (p==_period){

        foreach (GtBuffer*B, l_out_buffers) {
            if (mB2A[B]!=B->A){
                B->timeDataChanged=T;
                if (l_buffers4send.size()<5) l_buffers4send.push_back(B);
            };
        }
        // 1 тип шлем всегда по нему жизнеспособность опр
        foreach (GtBuffer*B, l_out_buffers) {
            if (B->getType()==1){
                if ((!B->timeDataRecived.isValid())||(( B->timeDataRecived.msecsTo(T)>=B->msecPeriodLive)&&(B->msecPeriodLive>0))){
                    if (!l_buffers4send.contains(B)) l_buffers4send.push_back(B);
                }
            }
        }

        foreach (GtBuffer*B, l_out_buffers) {
            if (l_buffers4send.size()<5){
                if ((!B->timeDataRecived.isValid())||((B->timeDataRecived.msecsTo(T)>=B->msecPeriodLive)&&(B->msecPeriodLive>0))){
                    if (!l_buffers4send.contains(B)) l_buffers4send.push_back(B);
                }
            }
        }
        GtBuffer*B=oldestBuffer(l_out_buffers);
        if (B!=nullptr) {
            if ((!B->timeDataRecived.isValid())||((B->timeDataRecived.msecsTo(T)>=B->msecPeriodLive)&&(B->msecPeriodLive>0))){
                l_buffers4send.push_back(B);
            }

        }
    }

    foreach (GtBuffer*B, l_buffers4send) {
        B->timeDataRecived=T;
        mB2A[B]=B->A;
        udp->sendGtBuffer(B);
    }

}

void UVK_Central::checkFinishRospusk(const QDateTime &T)
{
    bool STATE_GAC_FINISH=false;
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk){
        bool noendex=false;
        // ищем все конченые отцепы
        foreach (auto otcep, otcepsController->otceps->otceps()) {
            if (otcep->STATE_ENABLED()){
                if ((otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) ||(otcep->STATE_GAC_ACTIVE()==1)){
                    noendex=true;
                    break;
                }
                if ((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk) && (otcep->STATE_V()==0) && (otcep->STATE_GAC_ACTIVE()!=1)){
                    noendex=true;
                    break;
                }
            }
        }

        if (!noendex) STATE_GAC_FINISH=true;

    } else {
        if (t_STATE_GAC_FINISH.isValid()) t_STATE_GAC_FINISH=QDateTime();
    }
    if ((GORKA->STATE_GAC_FINISH()==false) && (STATE_GAC_FINISH==true)) t_STATE_GAC_FINISH=T;
    GORKA->setSTATE_GAC_FINISH(STATE_GAC_FINISH);

    if (STATE_GAC_FINISH==true){
        if ((!t_STATE_GAC_FINISH.isValid())||(t_STATE_GAC_FINISH.msecsTo(T)>=1*60*1000)){
            QString acceptStr;
            if (testMode==0){
                cmd_setRegim(ModelGroupGorka::regimStop,acceptStr);
            }
        }
    }


}


int UVK_Central::getNewOtcep(m_RC*rc,int drobl)
{
    if (GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk){
        if (maxOtcepCurrenRospusk+1>otcepsController->otceps->l_otceps.size()) return 0;
        // проверим что едем с нужной зкр
        foreach (auto zkr, l_zkr) {
            if ((rc==zkr) && (zkr->STATE_ROSPUSK()==1)){
                auto otcep_first=otcepsController->otceps->l_otceps.first();
                ID_ROSP=otcep_first->STATE_ID_ROSP();
                if (drobl>0){
                    // ищем пред отцеп
                    if (maxOtcepCurrenRospusk>=1){
                        auto pred_otcep=otcepsController->otceps->otcepByNum(maxOtcepCurrenRospusk);
                        if ((pred_otcep!=nullptr)&&(pred_otcep->STATE_SL_VAGON_CNT()>1)){
                            if ((pred_otcep->STATE_ZKR_VAGON_CNT()>0)&&(pred_otcep->STATE_ZKR_VAGON_CNT()<pred_otcep->STATE_SL_VAGON_CNT())){
                                auto new_otcep=otcepsController->inc_otcep_drobl(maxOtcepCurrenRospusk,
                                                                                 pred_otcep->STATE_SL_VAGON_CNT()-pred_otcep->STATE_ZKR_VAGON_CNT());
                                if (new_otcep!=nullptr) {
                                    zkr->setSTATE_OTCEP_VAGADD(true);
                                    new_otcep->setSTATE_ENABLED(true);
                                    new_otcep->setSTATE_ID_ROSP(ID_ROSP);
                                    new_otcep->setSTATE_GAC_ACTIVE(1);
                                    maxOtcepCurrenRospusk=new_otcep->NUM();
                                    auto next_otcep=otcepsController->otceps->otcepByNum(maxOtcepCurrenRospusk+1);
                                    if (next_otcep!=nullptr) next_otcep->setSTATE_GAC_ACTIVE(0);
                                    return new_otcep->NUM();
                                }
                            }

                        }
                    }
                }

                m_Otcep *otcep=otcepsController->otceps->topOtcep();
                if (otcep!=nullptr)  {
                    if (otcep->NUM()>maxOtcepCurrenRospusk) maxOtcepCurrenRospusk=otcep->NUM();
                    if (otcep->STATE_MAR()>0) otcep->setSTATE_GAC_ACTIVE(1);
                    if (otcep->STATE_ID_ROSP()==0) otcep->setSTATE_ID_ROSP(ID_ROSP);
                    return otcep->NUM();
                }
                otcep=otcepsController->otceps->otcepByNum(maxOtcepCurrenRospusk+1);
                if (otcep!=nullptr)  {
                    otcep->resetStates();
                    otcep->setSTATE_ENABLED(true);
                    otcep->setSTATE_ID_ROSP(ID_ROSP);
                    if (otcep->NUM()>maxOtcepCurrenRospusk) maxOtcepCurrenRospusk=otcep->NUM();
                    return otcep->NUM();
                }



            }
        }

    }
    return 0;
}

int UVK_Central::resetOtcep2prib(int num)
{
    if ((GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk)) {
        auto otcep=otcepsController->otceps->otcepByNum(num);
        if (otcep!=nullptr){
            TOS->resetTracking(num);
            otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);
            otcep->setSTATE_ZKR_TLG(0);
            otcep->setSTATE_ZKR_VES(0);
            otcep->setSTATE_ZKR_OSY_CNT(0);
            otcep->setSTATE_ZKR_VAGON_CNT(0);
            otcep->setSTATE_ZKR_BAZA(0);
            return num;
        }
    }
    return 0;
}

int UVK_Central::nerascep(int num)
{
    if ((GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk)) {
        auto otcep=otcepsController->otceps->otcepByNum(num);
        auto otcep_next=otcepsController->otceps->otcepByNum(num+1);
        if ((otcep!=nullptr)&&(otcep_next!=nullptr)){
            if ((otcep_next->STATE_ENABLED()==1)&&
                    (otcep_next->STATE_LOCATION()==m_Otcep::locationOnPrib)&&
                    (otcep->STATE_SL_VAGON_CNT()>0)&&
                    (otcep_next->STATE_SL_VAGON_CNT()>0)){
                if ((otcep->STATE_ZKR_VAGON_CNT()==otcep->STATE_SL_VAGON_CNT()+otcep_next->STATE_SL_VAGON_CNT())){

                    TOS->resetTracking(otcep_next->NUM());
                    otcep_next->setSTATE_LOCATION(m_Otcep::locationUnknow);
                    otcep_next->setSTATE_ERROR(1);
                    maxOtcepCurrenRospusk=otcep_next->NUM();
                    return num;
                }
            }
        }
    }
    return 0;
}

QList<SignalDescription> UVK_Central::acceptOutputSignals()
{

    QList<SignalDescription> l;
    GORKA->setSIGNAL_ROSPUSK(GORKA->SIGNAL_ROSPUSK().innerUse());   l << GORKA->SIGNAL_ROSPUSK();
    GORKA->setSIGNAL_PAUSA(GORKA->SIGNAL_PAUSA().innerUse());       l << GORKA->SIGNAL_PAUSA();
    GORKA->setSIGNAL_STOP(GORKA->SIGNAL_STOP().innerUse());         l << GORKA->SIGNAL_STOP();
    GORKA->setSIGNAL_GAC_FINISH(GORKA->SIGNAL_GAC_FINISH().innerUse());   l << GORKA->SIGNAL_GAC_FINISH();

    foreach (auto zkr, l_zkr) {
        zkr->setSIGNAL_ROSPUSK(zkr->SIGNAL_ROSPUSK().innerUse());   l<<zkr->SIGNAL_ROSPUSK();
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
    case ModelGroupGorka::regimPausa:   GORKA->SIGNAL_PAUSA().setValue_1bit(1); break;
    case ModelGroupGorka::regimStop:    GORKA->SIGNAL_STOP().setValue_1bit(1); break;
    }
    GORKA->SIGNAL_GAC_FINISH().setValue_1bit(GORKA->STATE_GAC_FINISH());
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
            setRegimStop();
            GORKA->setSTATE_REGIM(p);

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
            setRegimStop();
            GORKA->setSTATE_REGIM(p);
            return true;
        }
    } break;
    case ModelGroupGorka::regimStop:
    {
        switch (p) {
        case ModelGroupGorka::regimRospusk:

            setRegimRospusk();
            GORKA->setSTATE_REGIM(p);
            acceptStr=QString("Режим РОСПУСК установлен.");
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
        if  (zkr->svet()->STATE_J()==1) return ModelGroupGorka::regimRospusk;
    }
    return ModelGroupGorka::regimStop;
}

void UVK_Central::setRegimRospusk()
{
    otcepsController->finishLiveOtceps();
    state2buffer();
    sendBuffers(_changed);
    otcepsController->resetLiveOtceps();
    maxOtcepCurrenRospusk=0;
    TOS->resetTracking();
    GAC->resetStates();
    TOS->setSTATE_ENABLED(true);
    GAC->setSTATE_ENABLED(true);

}

void UVK_Central::setRegimStop()
{
    GAC->resetStates();
    GAC->setSTATE_ENABLED(false);
    ID_ROSP=0;
    // финишируем отцепы
    foreach (auto otcep, otcepsController->otceps->otceps()) {
        otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
        otcep->RCS=nullptr;otcep->RCF=nullptr;otcep->vBusyRc.clear();        otcep->setSTATE_GAC_ACTIVE(false);
    }

}


