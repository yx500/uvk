#ifndef TOS_ZKRTRACKING_H
#define TOS_ZKRTRACKING_H

#include "m_rc_gor_zkr.h"
#include "tos_dso.h"
#include "tos_dso_pair.h"
#include "tos_rctracking.h"



enum TZkrOtcepState{
    _zkrOtcepNone=0,
    _zkrOtcepEnter,
    _zkrOtcepLeave,
    _zkrOtcepIn
};

enum T_OTC_ZKR_cmd{
    _resetDSO_0=1000,
    _resetDSO_1,
    _resetDSO_0IF3,

    _in,
    cmdMove_A_ToFront_in,
    _error_rtds,_baza,
    _resetOtcepOnZKR
};

enum T_ZkrDsoState{
    _D0=0,
    _DF=1,
    _DB=2
};


class tos_ZkrTracking : public tos_RcTracking
{
    Q_OBJECT
public:
    struct t_zkr_state {
        TRcBOS rcos3[3];
        int rtds;
        int dso;

        bool likeTable(const t_zkr_state &ts) const{
            for (int j=0;j<3;j++){
                if (!rcOtcepStateLike(ts.rcos3[j],rcos3[j])) return false;
            }
            if ((ts.rtds!=_xx)||(ts.rtds!=rtds)) return false;
            if ((ts.dso!=_xx)||(ts.dso!=dso))return false;
            return true;
        }
        bool equal(const t_zkr_state &ts) const {
            for (int j=0;j<3;j++){
                if (!rcOtcepStateLike(ts.rcos3[j],rcos3[j])) return false;
            }
            if (ts.rtds!=rtds) return false;
            if (ts.dso!=dso)return false;
            return true;
        }
    };
    struct t_zkr_pairs{
        tos_ZkrTracking::t_zkr_state prev_state;
        tos_ZkrTracking::t_zkr_state curr_state;
        int cmd;
    };
public:

    explicit tos_ZkrTracking(tos_System_RC *parent, tos_Rc *rc);
    virtual ~tos_ZkrTracking(){}
    void resetStates()override;
    void setCurrState()override;
    void prepareCurState()override;
    void work(const QDateTime &T)override;

    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;



    m_RC_Gor_ZKR * rc_zkr;
signals:

public slots:
protected:

    tos_DSO *dsot[2][2];
    tos_DsoPair dso_pair;
    int osy_count[2];
    bool baza;
    qreal Vdso;
    QDateTime VdsoTime;

    t_zkr_state curr_state_zkr;
    t_zkr_state prev_state_zkr;

    void newOtcep(const QDateTime &T);
    void checkNeRascep(int num);
    void checkOsyCount(m_Otcep *otcep);
    void resetOtcep2Nadvig();

    int prev_base_os;
    int baza_count;
    void reset_dso(int n);
    void work_dso(const QDateTime &T);

    int _find_step(tos_ZkrTracking::t_zkr_pairs steps[],int size_steps,const tos_ZkrTracking::t_zkr_state &prev_state,const tos_ZkrTracking::t_zkr_state &curr_state);
    void doCmd(int cmd,const QDateTime &T)override ;



};

//Q_DECLARE_METATYPE(T_OTC_ZKR_sost)

#endif // TOS_ZKRTRACKING_H
