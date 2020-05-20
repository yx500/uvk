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
    Q_INVOKABLE TrackingOtcepSystem(QObject *parent,ModelGroupGorka *modelGorka,int trackingType=0);
    virtual ~TrackingOtcepSystem(){}
    virtual QList<BaseWorker *> makeWorkers(ModelGroupGorka *O);
    virtual void work(const QDateTime &T);

    virtual void resetStates();
    void updateOtcepsOnRc();


    QList<m_Otcep *> lo;
    m_Otceps *otceps;
    ModelGroupGorka *modelGorka=nullptr;
    QList<tos_ZkrTracking *> l_zkrt;
    void state2buffer();
    void disableBuffers();
    int trackingType=0;

    QList<tos_KzpTracking *> l_kzpt;

    QList<tos_RcTracking *> l_rct;

    QMap<m_RC*,m_Zam*> mRc2Zam;
    QMap<m_RC*,m_RIS*> mRc2Ris;

signals:
    void otcep_rcsf_change(m_Otcep *otcep,int sf,m_RC*rcFrom,m_RC*rcTo,QDateTime T,QDateTime Trc);

public slots:
protected:
    m_Otcep *topOtcep() const;

    int _regim=0;





};

#endif // TRACKINGOTCEPSYSTEM_H
