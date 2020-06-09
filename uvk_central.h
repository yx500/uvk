#ifndef UVK_CENTRAL_H
#define UVK_CENTRAL_H
#include <QTimer>
#include "trackingotcepsystem.h"
#include "gtgac.h"
#include "otcepscontroller.h"
#include "gtcommandinterface.h"
#include "modelgroupgorka.h"
#include "gtbuffers_udp_d2.h"
#include "mvp_system.h"


class UVK_Central : public QObject
{
    Q_OBJECT
public:
    explicit UVK_Central(QObject *parent = nullptr);
    bool init(QString fileNameModel);
    void start();
    ModelGroupGorka *GORKA;
    TrackingOtcepSystem*TOS;
    GtGac *GAC;
    OtcepsController *otcepsController;
    GtCommandInterface *CMD;
    GtBuffers_UDP_D2 *udp;
    QTimer *timer;
    QStringList errLog;
    void err(QString errstr);
    bool validation();
    bool acceptBuffers();
    bool cmd_setPutNadvig(int p,QString &acceptStr);
    bool cmd_setRegim(int p,QString &acceptStr);


    QList<GtBuffer*> l_out_buffers;
    QMap<GtBuffer*,QByteArray> mB2A;
    void state2buffer();

    void sendBuffers();

    virtual QList<SignalDescription> acceptOutputSignals() ;



signals:

public slots:
     void work();
     void recv_cmd(QMap<QString,QString> m);
     void gac_command(const SignalDescription&s,int state);
protected:
};

#endif // UVK_CENTRAL_H
