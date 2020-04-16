#ifndef TOS_OTCEPDATA_H
#define TOS_OTCEPDATA_H

#include "baseobject.h"

#include "m_otcep.h"
#include "tos_rctracking.h"

class TrackingOtcepSystem;




struct TTXV{
    QDateTime t;
    qreal x;
    qreal v;
    TTXV(){t=QDateTime();v=0;x=0;}
    TTXV(QDateTime _t,qreal _x,qreal _v){t=_t;x=_x;v=_v;}
};






class tos_OtcepData : public BaseObject
{
    Q_OBJECT
public:

    MYSTATE(qreal, STATE_V_RCS)
    MYSTATE(qreal, STATE_V_RCF)

public:
    explicit tos_OtcepData(TrackingOtcepSystem *parent, m_Otcep *otcep);
    virtual  ~tos_OtcepData(){}

    virtual void resetStates();

    m_Otcep *otcep;


    qreal STATE_V() const;
    void calcLenByRc();

    void state2buffer();



    // для TOS

    bool rc_tracking_comleted[2];

    QDateTime dtRCS;
    QDateTime dtRCF;

    // Dynamic
    QDateTime dos_dt_RCS_XOFFSET;
    QDateTime dos_dt_RCF_XOFFSET;
    m_RC*dos_RCS;
    m_RC*dos_RCF;

    void setOtcepSF(m_RC* rcs, m_RC* rcf, const QDateTime &T);

    void setCurrState();





signals:

public slots:

protected:
    TrackingOtcepSystem *TOS;
};

#endif // TOS_OTCEPDATA_H
