#ifndef TOS_RC_H
#define TOS_RC_H

#include "baseworker.h"
#include "m_rc.h"
#include "m_otcep.h"

class TrackingOtcepSystem;
class tos_OtcepData;

struct TOtcepData{
    TOtcepData(){this->p=_pOtcepPartUnknow;this->num=0;track=false;d=0;}
    TOtcepData(TOtcepPart p,int num){this->p=p;this->num=num;track=false;d=0;}
    TOtcepData(const TOtcepData&od){this->p=od.p;this->num=od.num;track=od.track;d=od.d;}
    TOtcepPart p;
    int num;
    bool track;
    int d;
    bool operator == (const TOtcepData& r) const {
        return !memcmp(this, &r, sizeof(r));
    }
    bool operator != (const TOtcepData& r) const {
        return memcmp(this, &r, sizeof(r));
    }
};



class tos_Rc : public BaseWorker
{
public:
    tos_Rc(TrackingOtcepSystem *TOS,m_RC *rc);
    m_RC *rc;

    void work(const QDateTime &T) override;
    void resetStates()override;

    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;

    TOtcepData od(int sf) const;
    QList<TOtcepData> l_od;
    MVP_Enums::TRCBusy STATE_BUSY;
    bool STATE_ERR_LS;
    bool STATE_ERR_LZ;
    bool STATE_ERR_KZ;
    bool STATE_CHECK_FREE_DB;
    QList<int> l_otceps;
    tos_Rc *next_rc[2];
    bool useRcTracking;

    tos_OtcepData *otcepOnRc(int sf);
    void remove_od(TOtcepData o);

protected:
    TrackingOtcepSystem *TOS;
    QDateTime time_STATE_BUSY;

};

#endif // TOS_RC_H
