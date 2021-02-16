#ifndef TOS_SYSTEM_DSO_H
#define TOS_SYSTEM_DSO_H

#include "tos_system.h"
#include "tos_dso.h"
#include "tos_dsotracking.h"
#include "tos_zkr_dso.h"
#include "tos_dso_pair.h"

class tos_System_DSO : public tos_System
{
    Q_OBJECT
public:
    explicit tos_System_DSO(QObject *parent);
    virtual ~tos_System_DSO(){}


    void validation(ListObjStr *l) const override;
    void makeWorkers(ModelGroupGorka *modelGorka) override;
    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;
    void work(const QDateTime &T)override;
    void resetStates()override;

    void updateOtcepsOnRc(const QDateTime &T);
    void resetTracking(int num)override;
    void resetTracking()override;
    bool resetDSOBUSY(QString idtsr, QString &acceptStr)override;

    void reset_1_os(const QDateTime &T);

    void setDSOBUSY(const QDateTime &T);
    void setNGBDYN(const QDateTime &T);

    TOtcepDataOs moveOs(tos_Rc *rc0, tos_Rc *rc1, int d, qreal os_v, const QDateTime &T);


    int zkr_id;

    QMap<m_DSO*,tos_DSO*> mDSO2TDSO;
    QMap<m_DSO*,tos_DsoTracking*> mDSO2TRDSO;
    QList<m_DSO_RD_21*> l_dso;
    QList<tos_DSO*> l_tdso;
    QList<tos_DsoTracking*> l_trdso;
    QList<tos_DsoPair*> l_tdsopair;
    QList<tos_Zkr_DSO*> l_tzkr;
    QList<tos_Rc*> l_trc_park;
    tos_Rc * getRc(TOtcepDataOs os);

signals:

public slots:
protected:

    void set_otcep_STATE_WARN(const QDateTime &T);







};



#endif // TOS_SYSTEM_DSO_H
