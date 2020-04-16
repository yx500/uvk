#ifndef TRACKINGOTCEPSYSTEM_H
#define TRACKINGOTCEPSYSTEM_H

#include "baseworker.h"

#include "m_otceps.h"
#include "m_rc.h"
#include "m_rc_gor_zkr.h"
#include "m_rc_gor_park.h"
#include "modelgroupgorka.h"
#include "tos_kzptracking.h"
#include "tos_zkrtracking.h"
#include "tos_rctracking.h"
#include "tos_otcepdata.h"


class tos_KzpTracking;
class tos_SpeedCalc;
class m_Zam;


class TrackingOtcepSystem : public BaseWorker
{
    Q_OBJECT

public:
    Q_INVOKABLE TrackingOtcepSystem(QObject *parent,ModelGroupGorka *modelGorka);
    virtual ~TrackingOtcepSystem(){}
    virtual QList<BaseWorker *> makeWorkers(ModelGroupGorka *O);
    virtual void work(const QDateTime &T);

    virtual void resetStates();
    void updateOtcepsOnRc();
    void checkOtcepComplete(m_Otcep *otcep);
    void updateOtcepParams(m_Otcep *otcep);
    void updateV_RC(m_Otcep *otcep, const QDateTime &T);



    QList<m_Otcep *> lo;
    m_Otceps *otceps;
    ModelGroupGorka *modelGorka=nullptr;
    QList<tos_ZkrTracking *> l_zkrt;
    void state2buffer();

signals:

public slots:
protected:
    m_Otcep *topOtcep() const;
    QList<tos_KzpTracking *> l_kzpt;

    QList<tos_RcTracking *> l_rct;

    QMap<m_RC*,m_Zam*> mRc2Zam;





};

#endif // TRACKINGOTCEPSYSTEM_H
