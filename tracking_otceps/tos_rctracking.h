#ifndef TOS_RCTRACKING_H
#define TOS_RCTRACKING_H

#include "baseworker.h"
#include "tos_rc.h"
#include "m_otcep.h"
#include <QElapsedTimer>



class tos_System_RC;

enum {_1od=0,_allod=1};

class tos_RcTracking : public BaseWorker
{
    Q_OBJECT
public:

    enum T_RcOtcepState{
        ___=-7,
        _NN=-1,
        _T0=0,
        _TS=10,
        _TF=20,
        _TM=30,
        _TSF=40,

        _tx=110,

        _Tx=910,
        _TSx=920,


        _xx=999
    };
    enum T_RcBusyState{
        _B0=0,_B1=1,
        _LS=10,
        _LZ=20,
        _KZ0=30,_KZ1=31
    };
    struct TRcBOS{
        TRcBOS(){BS=0;OS=0;}
        TRcBOS(int B,int O){BS=B;OS=O;}
        int BS;
        int OS;
        bool equal(const TRcBOS&s) const {
            if ((BS!=s.BS)||(OS!=s.OS)) return false;
            return true;
        }
    };


    static bool rcOtcepStateLike(const TRcBOS  &TS, const TRcBOS  &S){
        if ((TS.BS==S.BS)&&(TS.OS==S.OS)) return true;
        if ((TS.BS==_xx) && (TS.OS==_xx))return true;
        if ((TS.BS!=_xx) && (TS.BS!=S.BS)) return false;
        if (TS.OS==_xx) return true;
        if (TS.OS==S.OS) return true;
        if ((TS.OS==_Tx)&&((S.OS==_TS)||(S.OS==_TF)||(S.OS==_TM)||(S.OS==_TSF))) return true;
        if ((TS.OS==_TSx)&&((S.OS==_TS)||(S.OS==_TSF))) return true;
        return false;
    }
    static bool rcOtcepStateLikePrev(const TRcBOS  &TS, const TRcBOS  &S,const TRcBOS  &prevS){
        TRcBOS SS=S;
        if (TS.BS==___) SS.BS=prevS.BS;
        if (TS.OS==___) SS.OS=prevS.OS;
        return  rcOtcepStateLike(TS,SS);
    }

    enum T_tos_RcTrackingCommand{
        _none=0,
        cmdMove_1_ToFront,
        cmdMove_A_ToFront,
        cmdMove_1_ToBack,
        cmdMove_A_ToBack,

        cmdMove_A_ToFront_E,
        cmdMove_A_ToBack_E,
        cmdMove_A_ToFront_Nagon,

        cmdMove_A_ToFront2,
        cmdMove_1_ToFront2,
        cmdMove_1_ToFront2_LS,
        cmdMove_1_ToFront2_LZ,
        cmdMove_1_ToFront2_KZ,
        cmd_Reset
    };

    int otcepState(const TOtcepData &od1);
    int busyState(const tos_Rc *trc);
    TRcBOS busyOtcepStaotcepteM(const tos_Rc *trc, int sf);
    TRcBOS busyOtcepStaotcepteN(const tos_Rc *trc,int sf,int main_num);


    struct t_rc_tracking_state{
        TRcBOS rcos4[4];
        bool likeTable(const t_rc_tracking_state &ts) const{
            for (int j=0;j<4;j++){
                if (!rcOtcepStateLike(ts.rcos4[j],rcos4[j])) return false;
            }
            return true;
        }
        bool likeTablePrev(const t_rc_tracking_state &ts,const t_rc_tracking_state &prev_s) const{
            for (int j=0;j<4;j++){
                if (!rcOtcepStateLikePrev(ts.rcos4[j],rcos4[j],prev_s.rcos4[j])) return false;
            }
            return true;
        }

        bool equal(const t_rc_tracking_state &ts) const {
            for (int j=0;j<4;j++){
                if (!ts.rcos4[j].equal(rcos4[j])) return false;
            }
            return true;
        }
    };
    Q_ENUM(T_RcOtcepState)


    struct t_rc_pairs{
        t_rc_tracking_state prev_state;
        t_rc_tracking_state curr_state;
        T_tos_RcTrackingCommand cmd;
    };

public:
    explicit tos_RcTracking(tos_System_RC *parent,tos_Rc *trc);
    virtual  ~tos_RcTracking(){}

    void validation(ListObjStr *l) const;

    virtual void workOtcep(int dd, const QDateTime &T, int stepTable);

    virtual void work(const QDateTime &T);
    virtual void setCurrState();
    virtual void prepareCurState();
    tos_Rc * trc;

    virtual void resetStates();


    tos_Rc * curr_rc3[3];
    tos_Rc * prev_rc3[3];
    t_rc_tracking_state prev_state[2];
    t_rc_tracking_state curr_state[2];

    bool rc_tracking_comleted;





signals:

public slots:
protected:
    tos_System_RC *TOS;
    void workLSLZ(const QDateTime &T,int lslz);
    void moveOd(tos_Rc*next_trc,int _1a, int d, const QDateTime &T, bool bnorm);
    virtual void doCmd(int cmd,const QDateTime &T);
    void setOtcepNagon(int num);
    void setOtcepErrorTrack(int num);


};
//Q_DECLARE_METATYPE(T_BusyOtcepState)
#endif // TOS_RCTRACKING_H
