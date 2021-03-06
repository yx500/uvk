#ifndef OTCEPSCONTROLLER_H
#define OTCEPSCONTROLLER_H

#include "baseworker.h"
#include "m_otceps.h"

class OtcepsController : public BaseWorker
{
    Q_OBJECT
public:
    explicit OtcepsController(QObject *parent,m_Otceps *otceps);
    m_Otceps *otceps;
    virtual ~OtcepsController()override{}
    void resetStates() override;
    void work(const QDateTime &T) override;
    void validation(ListObjStr *l) const override;
    QList<SignalDescription> acceptOutputSignals() override;

    void state2buffer()override;

    bool cmd_CLEAR_ALL(QString &acceptStr);
    bool cmd_ACTIVATE_ALL(QString &acceptStr);
    bool cmd_UPDATE_ALL(QString &acceptStr);
    bool cmd_DEL_OTCEP(QMap<QString, QString> &m,QString &acceptStr);
    bool cmd_INC_OTCEP(QMap<QString, QString> &m,QString &acceptStr);
    bool cmd_SET_OTCEP_STATE(QMap<QString, QString> &m,QString &acceptStr);
    bool cmd_SET_VAGON_STATE(QMap<QString, QString> &m,QString &acceptStr);
//    bool cmd_ADD_OTCEP_VAG(QMap<QString, QString> &m,QString &acceptStr);
    bool cmd_SET_CUR_OTCEP(QMap<QString, QString> &m,QString &acceptStr);
    bool cmd_CHECK_LIST(QMap<QString, QString> &m,QString &acceptStr);

    void resetLiveOtceps();
    void finishLiveOtceps();

    void setNewID_ROSP(quint32 ID_ROSP);

    void updateVagons();
    m_Otcep *inc_otcep(int N, int sp);
    m_Otcep *inc_otcep_drobl(int N);
    m_Otcep *nerascep(int num);
    QList<SignalDescription> l_chanelVag;
//    QMap<m_Otcep*,SignalDescription> mO29;
//    QMap<m_Otcep*,SignalDescription> mO2109;
signals:

};

#endif // OTCEPSCONTROLLER_H
