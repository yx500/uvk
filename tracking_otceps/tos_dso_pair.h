#ifndef TOS_DSO_PAIR_H
#define TOS_DSO_PAIR_H

#include "tos_dso.h"
#include "tos_rc.h"


struct TTLG_record{
    int os_start=0;
    int tlg_os=0;

};


class tos_DsoPair
{
    public:
    enum T_ZKR_TLG_sost{
        tlg_0=0,tlg_wait1=1,tlg_wait2=2,tlg_error=-1
    };
    tos_DsoPair();
    int d;
    TTLG_record c;
    T_ZKR_TLG_sost sost;

    QList<TTLG_record> l_tlg;
    int tlg_cnt;

    void updateStates(int os1,int os2);
    void resetStates();


//    Q_ENUM(T_ZKR_TLG_sost)
};


#endif // TOS_DSO_PAIR_H
