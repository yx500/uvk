#ifndef TOS_OTCEPDATA_H
#define TOS_OTCEPDATA_H

#include "baseobject.h"

#include "m_otcep.h"
#include "tos_rctracking.h"

class TrackingOtcepSystem;




class tos_OtcepData : public BaseObject
{
    Q_OBJECT
public:

    MYSTATE(qreal, STATE_V_RCF_IN)
    MYSTATE(qreal, STATE_V_RCF_OUT)
    MYSTATE(qreal, STATE_LEN_BY_RC_MIN)
    MYSTATE(qreal, STATE_LEN_BY_RC_MAX)

public:
    explicit tos_OtcepData(TrackingOtcepSystem *parent, m_Otcep *otcep);
    virtual  ~tos_OtcepData(){}

    virtual void resetStates();

    m_Otcep *otcep;

    qreal STATE_V() const;
    void updateV_RC(const QDateTime &T);
//    void calcLenByRc();

    QDateTime dtRCS;
    QDateTime dtRCF;

    // Dynamic
    QDateTime dos_dt_RCS_XOFFSET;
    QDateTime dos_dt_RCF_XOFFSET;
    m_RC*dos_RCS;
    m_RC*dos_RCF;
    int _stat_kzp_d=0;
    QDateTime _stat_kzp_t;

    qreal rcf_v_pred;

    void setOtcepSF(int sf, m_RC *rcTo, int d, const QDateTime &T, bool bnorm);
    void resetTracking();

signals:

public slots:

protected:
    TrackingOtcepSystem *TOS;
};

#endif // TOS_OTCEPDATA_H
