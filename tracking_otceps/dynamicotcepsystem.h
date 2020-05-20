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
    void calculateXoffset(m_Otcep *otcep, const QDateTime &T, int sf);
    void calculateXoffsetKzp(const QDateTime &T);

    DynamicStatistic *dynamicStatistic=nullptr;


signals:

public slots:
    void slot_otcep_rcsf_change(m_Otcep *otcep,int sf,m_RC*rcFrom,m_RC*rcTo,QDateTime T,QDateTime Trc);

protected:
    TrackingOtcepSystem *TOS;

    QList<m_RC_Gor_Park*> l_rc_park;
};

#endif // DYNAMICOTCEPSYSTEM_H
