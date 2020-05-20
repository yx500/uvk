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
    void disableBuffers()override;
    void state2buffer()override;

    bool cmd_CLEAR_ALL(QMap<QString, QString> &m);
    bool cmd_ACTIVATE_ALL(QMap<QString, QString> &m);
    bool cmd_DEL_OTCEP(QMap<QString, QString> &m);
    bool cmd_INC_OTCEP(QMap<QString, QString> &m);
    bool cmd_SET_OTCEP_STATE(QMap<QString, QString> &m);
    bool cmd_ADD_OTCEP_VAG(QMap<QString, QString> &m);
signals:

};

#endif // OTCEPSCONTROLLER_H
