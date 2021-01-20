#ifndef DYNAMICOTCEPSYSTEM_H
#define DYNAMICOTCEPSYSTEM_H

#include "trackingotcepsystem.h"
class m_RIS;
class m_RC_Gor_Park;
class DynamicStatistic;


class DynamicOtcepSystem : public BaseWorker
{
    Q_OBJECT
public:
    explicit DynamicOtcepSystem(QObject *parent,TrackingOtcepSystem *TOS);
    virtual ~DynamicOtcepSystem(){}
    virtual void work(const QDateTime &T);
    virtual void resetStates();
    void calculateXoffset(tos_OtcepData *o, const QDateTime &T, int sf);
    void calculateXoffsetKzp(const QDateTime &T);

    DynamicStatistic *dynamicStatistic=nullptr;


signals:

public slots:

protected:
    TrackingOtcepSystem *TOS;
};

#endif // DYNAMICOTCEPSYSTEM_H
