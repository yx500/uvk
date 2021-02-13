#ifndef TOS_DSO_H
#define TOS_DSO_H

#include "m_dso_rd_21.h"
#include "baseworker.h"

enum {_os_none=-1,_os2forw=0,_os2back=1};
class tos_DSO : public BaseWorker
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
        _none=0,_inc_os,_dec_os,_sboy,_inc_os_sboy,_dec_os_sboy,_sboy_last_d
    };
    public:

        explicit tos_DSO(QObject *parent,m_DSO_RD_21 *dso);
    virtual ~tos_DSO(){}
    void resetStates() override;
    void work(const QDateTime &T) override;
    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;

    void resetTracking();

    TDSO_statistic getStatistic(qlonglong n);

    m_DSO_RD_21 *dso;

    int os_moved;
    qreal os_v;
signals:

public slots:
protected:
    int current_sost;

    void inc_os(int p, QDateTime T);

    QList<TDSO_statistic> l_statistic;

};


#endif // TOS_DSO_H
