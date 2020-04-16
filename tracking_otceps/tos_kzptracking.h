#ifndef TOS_KZPTRACKING_H
#define TOS_KZPTRACKING_H

// определяет заезд отцепа в парк
// отслеживает и прогнозирует движение отцепов по пути парка

#include "m_rc_gor_park.h"
#include "baseworker.h"
#include "tos_rctracking.h"

class TrackingOtcepSystem;
class m_RC_Gor_Park;
class m_RC;
class m_Otcep;

enum TZkpOtcepState{
    _kzpOtcep_None=0,
    _kzpOtcep_RCS_EnterPark,
    _kzpOtcep_RCF_EnterPark,
    _kzpOtcep_RCS_LeavePark,
    _kzpOtcep_RCF_LeavePark
};

enum T_kzp_rc_state{
    _NNN=-1,


    _MMT0,
    _MMTS,
    _MMTF,

    _LLT0,
    _LLTS,
    _LLTF,

    _LMT0,
    _LMTS,
    _LMTF,

    _MLT0,
    _MLTS,
    _MLTF

    //,_xxxx=99
};


enum T_kzp_state{
    s_ok
};


class tos_KzpTracking : public tos_RcTracking
{
    Q_OBJECT
public:
    struct t_kzp_tracking_state{
        T_kzp_state flag;
        T_kzp_rc_state kzpos;
        T_BusyOtcepState rcos0;
        bool likeTable(const t_kzp_tracking_state &ts) const{
             if (ts.kzpos!=kzpos)   return false;
             if (!busyOtcepStateLike(ts.rcos0,rcos0)) return false;
            if ((ts.flag!=flag)) return false;
            return true;
        }
    };
    enum T_kzp_command{
        cmdNothing=0,
        cmdMoveStartToPark,
        cmdMoveFinishToPark,
        cmdMoveStartToRc0,
        cmdMoveStartFinishToPark,
        cmdUpdateLast,
        cmdReset
    };
    struct t_kzp_pairs{
        t_kzp_tracking_state prev_state;
        t_kzp_tracking_state curr_state;
        T_kzp_command cmd;
    };

    explicit tos_KzpTracking(QObject *parent ,m_RC_Gor_Park * rc_park);
    virtual ~tos_KzpTracking(){}
    virtual void resetStates();

    m_RC_Gor_Park * rc_park;
    virtual void work(const QDateTime &T);

signals:


protected:
    m_RC *rc2[2];

    t_kzp_tracking_state prev_state_kzp;
    t_kzp_tracking_state curr_state_kzp;


    int prev_kzp;
    int x_narast;
    QDateTime dtNarast;


    T_kzp_rc_state kzp_rc_state();

};

#endif // TOS_KZPTRACKING_H
