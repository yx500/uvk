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

class IGetNewOtcep
{
    public:
    virtual int getNewOtcep(m_RC*rc)=0;
};

class TrackingOtcepSystem : public BaseWorker
{
    Q_OBJECT

public:
    Q_INVOKABLE TrackingOtcepSystem(QObject *parent,ModelGroupGorka *modelGorka,int trackingType=0);
    virtual ~TrackingOtcepSystem(){}

    void setIGetNewOtcep(IGetNewOtcep *i){iGetNewOtcep=i;}
    tos_OtcepData *getNewOtcep(tos_Rc *trc);

    virtual QList<BaseWorker *> makeWorkers(ModelGroupGorka *O);
    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;
    void work(const QDateTime &T)override;
    void resetStates()override;


    void updateOtcepsOnRc(const QDateTime &T);
    void updateOtcepParams(tos_OtcepData *o, const QDateTime &T);
    void resetTracking(int num);
    void resetTracking();



    ModelGroupGorka *modelGorka=nullptr;
    int trackingType=0;

    QList<tos_OtcepData *> lo;
    QList<tos_Rc *> l_tos_Rc;

    QList<tos_ZkrTracking *> l_zkrt;
    QList<tos_KzpTracking *> l_kzpt;
    QList<tos_RcTracking *> l_rct;

    QMap<m_RC*,m_Zam*> mRc2Zam;
    QMap<m_RC*,m_RIS*> mRc2Ris;
    QMap<m_RC*,tos_Rc*> mRc2TRC;
    QMap<int,tos_OtcepData*> mNUM2OD;

signals:
    //void otcep_rcsf_changed(int num, int sf, tos_Rc *rcTo, int d,QDateTime T, bool bnorm);

public slots:
protected:
    IGetNewOtcep *iGetNewOtcep;

    int _regim=0;
    void checkOtcepComplete();
    void checkOtcepSplit();






};

#endif // TRACKINGOTCEPSYSTEM_H
