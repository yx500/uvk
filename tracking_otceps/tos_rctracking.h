#ifndef TOS_RCTRACKING_H
#define TOS_RCTRACKING_H

#include "baseworker.h"
#include "m_otcep.h"

enum {tos_normal=0,tos_hard=1};


class tos_RcTracking : public BaseWorker
{
    Q_OBJECT
public:
    enum T_BusyOtcepState{
        _NN=-1,
        ___B0=0,___B1=1,
        _TSB0=10,_TSB1=11,
        _TFB0=20,_TFB1=21,
        _TSFB0=30,_TSFB1=31,
        _TMB0=40,_TMB1=41,
        _tsB0=50,_tsB1=51,
        _tfB0=60,_tfB1=61,
        _tsfB0=70,_tsfB1=71,
        _tmB0=80,_tmB1=81,

        _xx=99,
        _xB0,_xB1,
        _TxB0,_txB0,
        _TxB1,_txB1
    };
    enum T_tos_RcTrackingCommand{
        _none=0,
        cmdMoveStartToFront,
        cmdMoveStartToBack,
        cmdMoveFinishToFront,
        cmdMoveFinishToFrontCheck,
        cmdMoveFinishToBack,
        cmdMoveFinishToRC0,
        cmdMoveStartFinishToRc1,
        cmdPushStartFinishToRc1,
        cmdCheckDrebezg0,
        cmdCheckDrebezg1onRC3,
        cmdSetLSStartToRc0,
        cmdSetLSStartFinishToRc0,
        cmdSetLZStartToRc0,
        cmdSetLZStartFinishToRc0,
        cmdSetKZStartToRc0,
        cmdResetRCF,
        cmdReset
    };



    static T_BusyOtcepState busyOtcepState(int state_busy,int otcep_num,TOtcepPart otcep_part,int otcep_main_num);
    static T_BusyOtcepState busyOtcepState(const m_RC *rc,const m_RC *rcMain) ;

    static bool busyOtcepStateLike(const T_BusyOtcepState &TS, const T_BusyOtcepState &S);

    enum t_rc_tracking_flag{
        s_none=0,
        s_checking_drebezg,
        s_checking_drebezg1OnRC3,
        s_xx=99
    };

    struct t_rc_tracking_state{
        t_rc_tracking_flag flag;
        T_BusyOtcepState rcos[4];
        bool likeTable(const t_rc_tracking_state &ts) const{
            for (int j=0;j<4;j++){
                if (!busyOtcepStateLike(ts.rcos[j],rcos[j])) return false;
            }
            if ((ts.flag!=s_xx)&&(ts.flag!=flag)) return false;
            return true;
        }
    };
    Q_ENUM(T_BusyOtcepState)

    MYSTATE(bool,STATE_CHECK_FREE_DB)


    MYSTATE(T_BusyOtcepState,STATE_S0)
    MYSTATE(T_BusyOtcepState,STATE_S1)
    MYSTATE(T_BusyOtcepState,STATE_S2)
    MYSTATE(T_BusyOtcepState,STATE_S3)
    MYSTATE(T_BusyOtcepState,STATE_P0)
    MYSTATE(T_BusyOtcepState,STATE_P1)
    MYSTATE(T_BusyOtcepState,STATE_P2)
    MYSTATE(T_BusyOtcepState,STATE_P3)

    struct t_rc_pairs{
        t_rc_tracking_state prev_state;
        t_rc_tracking_state curr_state;
        T_tos_RcTrackingCommand cmd;
    };

public:
    explicit tos_RcTracking(QObject *parent,m_RC *rc);
    virtual  ~tos_RcTracking(){}

    virtual void validation(ListObjStr *l) const;

    void workOtcep(int sf, const QDateTime &T, int stepTable);

    virtual void work(const QDateTime &T);
    void setCurrState();
    m_RC * rc;
    bool useRcTracking=false;

    virtual void resetStates();


    m_RC * curr_rc4[4];
    m_RC * prev_rc4[4];
    t_rc_tracking_state prev_state;
    t_rc_tracking_state curr_state;


    QList<m_Otcep *> l_otceps;
    void addOtcep(m_Otcep *otcep);
    void removeOtcep(m_Otcep *otcep=nullptr);

    // ГАЦ
    MVP_Enums::TStrelPol pol_zad;
    MVP_Enums::TStrelPol pol_cmd;
    MVP_Enums::TStrelPol pol_mar;
    QDateTime pol_cmd_time;
    QDateTime pol_cmd_w_time;

    void state2buffer();


signals:

public slots:
protected:
    void workLSLZ(const QDateTime &T, m_Otcep *otcep, bool sf, int lslz);
};
//Q_DECLARE_METATYPE(T_BusyOtcepState)
#endif // TOS_RCTRACKING_H
