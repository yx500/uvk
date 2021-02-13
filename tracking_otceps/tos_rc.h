#ifndef TOS_RC_H
#define TOS_RC_H

#include "baseworker.h"
#include "m_rc.h"
#include "m_otcep.h"

class tos_System;
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

struct TOtcepDataOs{
    TOtcepDataOs(){this->p=_pOtcepPartUnknow;this->num=0;os_otcep=0;d=0;zkr_num=0;v=_undefV_;t=QDateTime();}
    TOtcepDataOs(const TOtcepDataOs&od){this->p=od.p;this->num=od.num;os_otcep=od.os_otcep;d=od.d;zkr_num=od.zkr_num;v=od.v;t=od.t;}
    TOtcepPart p;
    int num;
    int d;
    int os_otcep;
    int zkr_num;
    qreal v;
    QDateTime t;
    bool operator == (const TOtcepDataOs& r) const {
        return !memcmp(this, &r, sizeof(r));
    }
    bool operator != (const TOtcepDataOs& r) const {
        return memcmp(this, &r, sizeof(r));
    }
};



class tos_Rc : public BaseWorker
{
public:
    tos_Rc(tos_System *TOS,m_RC *rc);
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

    QList<TOtcepDataOs> l_os;


protected:
    tos_System *TOS;
    QDateTime time_STATE_BUSY;

};

#endif // TOS_RC_H
