#ifndef TOS_ZKRTRACKING_H
#define TOS_ZKRTRACKING_H

#include "m_rc_gor_zkr.h"
#include "tos_dsotracking.h"
#include "tos_rctracking.h"

class ModelGroupGorka;
class m_Otceps;

enum TZkrOtcepState{
    _zkrOtcepNone=0,
    _zkrOtcepEnter,
    _zkrOtcepLeave,
    _zkrOtcepIn
};

enum T_OTC_ZKR_cmd{
     _none=0,_in,_error_rtds,_baza,
    _head2front,_back2front,_pushHeadBack2front,_resetOtcepOnZKR,_back2front_in
};



class tos_ZkrTracking : public tos_RcTracking
{
    Q_OBJECT
public:
    struct t_zkr_state {
        T_BusyOtcepState rcstate[3];
        int rtds;
        bool likeTable(const t_zkr_state &ts) const{
            for (int j=0;j<3;j++){
                if (!busyOtcepStateLike(ts.rcstate[j],rcstate[j])) return false;
            }
            if (ts.rtds!=rtds) return false;
            return true;
        }
    };
    struct t_zkr_pairs{
        tos_ZkrTracking::t_zkr_state prev_state;
        tos_ZkrTracking::t_zkr_state curr_state;
        int ext_state;
        T_OTC_ZKR_cmd cmd;
    };
public:
    MYSTATE(int ,STATE_LT_OSY_CNT)
    MYSTATE(int ,STATE_LT_OSY_S)
    MYSTATE(int ,STATE_TLG_CNT)
    MYSTATE(int, STATE_TLG_SOST)
    MYSTATE(int, STATE_TLG_D)
    MYSTATE(qreal, STATE_V_DSO)

    explicit tos_ZkrTracking(QObject *parent ,m_RC_Gor_ZKR * rc_zkr,m_Otceps *otceps,ModelGroupGorka *modelGorka);
    virtual ~tos_ZkrTracking(){}
    void resetStates()override;
    void work(const QDateTime &T)override;

    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;

    m_RC_Gor_ZKR * rc_zkr;
    ModelGroupGorka *modelGorka=nullptr;
signals:

public slots:
protected:
    m_Otceps *otceps;

    m_RC *rc_next;
    m_RC *rc_prev;
    tos_DsoTracking *dsot[2][2];
    tos_DsoPair dso_pair;
    int osy_count[2];
    qreal Vdso;
    QDateTime VdsoTime;

    t_zkr_state curr_state_zkr;
    t_zkr_state prev_state_zkr;

    void newOtcep(const QDateTime &T);
    void checkNeRascep();
    void checkOsyCount(m_Otcep *otcep);

    int prev_base_os;
    int baza_count;
    void reset_dso();
    void work_dso(const QDateTime &T);


};

//Q_DECLARE_METATYPE(T_OTC_ZKR_sost)

#endif // TOS_ZKRTRACKING_H
