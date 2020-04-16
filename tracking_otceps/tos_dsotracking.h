#ifndef TOS_DSOTRACKING_H
#define TOS_DSOTRACKING_H

#include "m_dso_rd_21.h"
#include "baseworker.h"


class tos_DsoTracking : public BaseWorker
{
    Q_OBJECT
public:
    struct TDSO_statistic{
        qlonglong OSY_COUNT;
        int DIRECT;
        QDateTime T;
    };
    enum T_DSO_sost{
        sost0=0,sostFree,sostOsEnter01,sostOsEnter10,sostOsEnter11,sostOsEnter0111,sostOsEnter1011,
        sostOsLeave10,sostOsLeave01
    };
    enum T_DSO_cmd{
        _none=0,_inc_os,_dec_os
    };
    public:

        explicit tos_DsoTracking(QObject *parent,m_DSO_RD_21 *dso);
    virtual ~tos_DsoTracking(){}
    virtual void resetStates();
    virtual void work(const QDateTime &T);
    void state2buffer();
    TDSO_statistic getStatistic(qlonglong n);

    m_DSO_RD_21 *dso;


signals:

public slots:
protected:
    int current_sost;
    void inc_os(int p, QDateTime T);

    QList<TDSO_statistic> l_statistic;

};

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


#endif // TOS_DSOTRACKING_H
