#ifndef TOS_SYSTEM_RC_H
#define TOS_SYSTEM_RC_H

#include "tos_system.h"


#include "tos_kzptracking.h"
#include "tos_zkrtracking.h"
#include "tos_rctracking.h"

class tos_KzpTracking;
class tos_SpeedCalc;
class m_Zam;


class tos_System_RC : public tos_System
{
    Q_OBJECT

public:
    explicit tos_System_RC(QObject *parent);
    virtual ~tos_System_RC(){}



    void makeWorkers(ModelGroupGorka *modelGorka) override;
    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer() override;
    void work(const QDateTime &T)override;
    void resetStates()override;


    void updateOtcepsOnRc(const QDateTime &T);
    void resetTracking(int num)override;
    void resetTracking()override;





    QList<tos_ZkrTracking *> l_zkrt;
    QList<tos_KzpTracking *> l_kzpt;
    QList<tos_RcTracking *> l_rct;



signals:
    //void otcep_rcsf_changed(int num, int sf, tos_Rc *rcTo, int d,QDateTime T, bool bnorm);

public slots:
protected:

    int _regim=0;
    void checkOtcepComplete();
    void checkOtcepSplit();






};



#endif // TOS_SYSTEM_RC_H
