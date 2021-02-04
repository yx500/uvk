#ifndef TOS_DSO_PAIR_H
#define TOS_DSO_PAIR_H

#include "tos_dso.h"
#include "tos_rc.h"
#include "tos_dsotracking.h"

class m_Strel_Gor_Y;


class tos_DsoPair : public tos_DsoTracking
{
public:
    enum {_sost_unknow,_sost_wait1t,_sost_wait2t};
    enum {_tlg_unknow,_tlg_forw,_tlg_back};

    explicit tos_DsoPair(tos_System_DSO *parent,tos_DSO *dso1,tos_DSO *dso2);
    virtual ~tos_DsoPair(){}
    void resetStates() override;
    void work(const QDateTime &T) override;

    int sost;
    int d_wait;
    int os_count;
    int os_n;

    int sost_teleg;
    TOtcepDataOs teleg_os;

//    enum T_ZKR_TLG_sost{
//        tlg_0=0,tlg_wait1=1,tlg_wait2=2,tlg_error=-1
//    };
//    struct TTLG_record{
//        int os_start=0;
//        int tlg_os=0;
//    };
//    int d;
//    TTLG_record c;
//    T_ZKR_TLG_sost sost;
//    QList<TTLG_record> l_tlg;
//    int tlg_cnt;
//    void updateStates_n(int os1,int os2);


    tos_DSO *dso1;
    tos_DSO *dso2;
    m_Strel_Gor_Y*strel;
    QList<TOtcepDataOs> l_os;
    void add_os(int d);


};


#endif // TOS_DSO_PAIR_H
