#ifndef TOS_ZKR_DSO_H
#define TOS_ZKR_DSO_H

#include "baseworker.h"
#include "tos_dso.h"
#include "tos_rc.h"
#include "m_rc_gor_zkr.h"


class tos_System_DSO;

enum {_otcep_unknow=-1, _otcep_free=0, _otcep_in,_xx=999};

enum T_OTC_ZKR_cmd{
    _none=0,
    _sost2free,
    _in,
    _otcep_end,
    _os_plus,
    _os_any_plus,
    _os_minus,
    _error_rtds
};



class tos_Zkr_DSO : public BaseWorker
{
    Q_OBJECT
public:
    explicit tos_Zkr_DSO(tos_System_DSO *parent,tos_Rc *rc);
    virtual ~tos_Zkr_DSO(){}
    void resetStates() override;

    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;

    void work(const QDateTime &T) override;

    tos_Rc *trc;
    m_RC_Gor_ZKR * rc_zkr;

    struct t_zkr_state {
        int sost;
        int rtds;
        int dso;
        int os_in;

        bool likeTable(const t_zkr_state &ts) const{
            if ((ts.rtds!=_xx)&&(ts.rtds!=rtds)) return false;
            if ((ts.dso!=_xx)&&(ts.dso!=dso))return false;
            if ((ts.sost!=_xx)&&(ts.sost!=sost)) return false;
            if ((ts.os_in!=_xx)&&(ts.os_in!=os_in)) return false;
            return true;
        }
        bool equal(const t_zkr_state &ts) const {
            if (ts.rtds!=rtds) return false;
            if (ts.dso!=dso)return false;
            if (ts.sost!=sost)return false;
            if ((ts.os_in!=os_in)) return false;
            return true;
        }
    };
    struct t_zkr_pairs{
        t_zkr_state prev_state;
        t_zkr_state curr_state;
        int cmd;
    };


signals:

public slots:
protected:
    tos_DSO *tdso[2][2];
    tos_System_DSO *TOS;
    tos_DSO * work_dso();

    TOtcepDataOs cur_os;

    t_zkr_state curr_state_zkr;
    t_zkr_state prev_state_zkr;

    void newOtcep(const QDateTime &T);
    void endOtcep(const QDateTime &T);
    void in_os(const QDateTime &T);
    void out_os(const QDateTime &T);


};

#endif // TOS_ZKR_DSO_H
