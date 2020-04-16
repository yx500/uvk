#ifndef DYNAMICOTCEPSYSTEM_H
#define DYNAMICOTCEPSYSTEM_H

#include "trackingotcepsystem.h"
class m_RIS;
class m_RC_Gor_Park;

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
    void updateV_ARS(m_Otcep *otcep, const QDateTime &T);
    void prognoz_KZP(m_Otcep *otcep, const QDateTime &T);


signals:

public slots:
protected:
    TrackingOtcepSystem *TOS;
    QMap<m_RC*,m_RIS*> mRc2Ris;
    QList<m_RC_Gor_Park*> l_rc_park;
};

#endif // DYNAMICOTCEPSYSTEM_H
