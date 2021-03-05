#ifndef UVK_CENTRAL_H
#define UVK_CENTRAL_H
#include <QTimer>
#include "tos_system.h"
#include "gtgac.h"
#include "otcepscontroller.h"
#include "gtcommandinterface.h"
#include "modelgroupgorka.h"
#include "gtbuffers_udp_d2.h"
#include "gtbuffers_memshared.h"

#include "mvp_system.h"

class gtapp_watchdog;

class UVK_Central : public QObject
{
    Q_OBJECT
public:
    explicit UVK_Central(QObject *parent = nullptr);
    bool init(QString fileNameIni);
    void start();
    ModelGroupGorka *GORKA;
    tos_System*TOS;
    GtGac *GAC;
    OtcepsController *otcepsController;
    GtCommandInterface *CMD;
    GtBuffers_UDP_D2 *udp;

    QStringList errLog;
    QStringList initLog;
    void err(QString errstr);
    void ilog(QString str);
    bool validation();
    bool acceptBuffers();
    bool cmd_setPutNadvig(int p,QString &acceptStr);
    bool cmd_setRegim(int p,QString &acceptStr);


    QList<GtBuffer*> l_out_buffers;
    QMap<GtBuffer*,QByteArray> mB2A;
    QList<m_RC_Gor_ZKR*> l_zkr;
    void state2buffer();



    int getNewOtcep(m_RC *rc, int drobl);
    int resetOtcep2prib(int num);

    virtual QList<SignalDescription> acceptOutputSignals() ;

    enum {_all,_changed,_period};
    void sendBuffers(int p);

    void checkFinishRospusk(const QDateTime &T);

signals:

public slots:
     void work();
     void sendBuffersPeriod();
     void recv_cmd(QMap<QString,QString> m);
     void gac_command(const SignalDescription&s, int state);
     void sendStatus();
protected:
     QTimer *timer_work;
     QTimer *timer_send;
     QList<GtBuffer*> l_buffers4send;
     SignalDescription signal_SlaveMode;
     SignalDescription signal_Control;
     int maxOtcepCurrenRospusk;
     bool slaveMode=false;

     QString fileNameModel;
     QString uvkStatusName;
     int trackingType;

     time_t start_time;

     quint32 ID_ROSP=0;

     int testRegim();
     void setRegimRospusk();
     void setRegimStop();
     gtapp_watchdog *watchdog;
     QStringList sl_memshared_buffers;
     QDateTime t_STATE_GAC_FINISH;
};

#endif // UVK_CENTRAL_H
