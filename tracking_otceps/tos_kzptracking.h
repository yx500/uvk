#ifndef TOS_KZPTRACKING_H
#define TOS_KZPTRACKING_H

// определяет заезд отцепа в парк
// отслеживает и прогнозирует движение отцепов по пути парка

#include "m_rc_gor_park.h"
#include "baseworker.h"
#include "tos_rctracking.h"

class tos_System_RC;
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

enum T_KzpBusyState{
    _MM=101,
    _LL,
    _LM,
    _ML
};



class tos_KzpTracking : public tos_RcTracking
{
    Q_OBJECT
public:
    struct t_kzp_tracking_state{
        TRcBOS rcos3[3];
        bool likeTable(const t_kzp_tracking_state &ts) const{
             for (int i=0;i<3;i++)
                if (!rcOtcepStateLike(ts.rcos3[0],rcos3[0])) return false;
            return true;
        }
    };
    enum T_kzp_command{
        cmdReset_kzp
    };
    struct t_kzp_pairs{
        t_kzp_tracking_state prev_state;
        t_kzp_tracking_state curr_state;
        int cmd;
    };

    explicit tos_KzpTracking(tos_System_RC *parent , tos_Rc *trc);
    virtual ~tos_KzpTracking(){}
    virtual void resetStates()override;

    void setCurrState() override;
    void prepareCurState()override;

    m_RC_Gor_Park * rc_park;
    virtual void work(const QDateTime &T)override;

signals:


protected:

    t_kzp_tracking_state prev_state_kzp;
    t_kzp_tracking_state curr_state_kzp;


    int prev_kzp;
    int curr_kzp;
    int x_narast;
    QDateTime dtNarast;


    TRcBOS kzp_rc_state();
    void doCmd(int cmd,const QDateTime &T) override;
    void closePred(const QDateTime &T);

};

#endif // TOS_KZPTRACKING_H
