#ifndef UVK_CENTRAL_H
#define UVK_CENTRAL_H
#include <QTimer>
#include "trackingotcepsystem.h"
#include "gtgac.h"
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
    GtCommandInterface *CMD;
    GtBuffers_UDP_D2 *udp;
    QTimer *timer;
    QStringList errLog;
    void err(QString errstr);
    bool validation();
    bool validateBuffers();
    void acceptBuffers();


    QList<GtBuffer*> l_buffers;
    QMap<GtBuffer*,QByteArray> mB2A;
    void sendBuffers();
    void acceptSignals();



signals:

public slots:
     void work();
     void recv_cmd(QMap<QString,QString> m);
protected:
     void setPutNadvig(int p);
     void setRegim(int p);
     void setOtcepStates(QMap<QString, QString> &m);
     void state2buffer();
};

#endif // UVK_CENTRAL_H
