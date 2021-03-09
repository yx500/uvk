#ifndef TOS_SYSTEM_H
#define TOS_SYSTEM_H

#include "baseworker.h"

#include "m_otceps.h"
#include "m_rc_gor_zkr.h"
#include "m_rc_gor_park.h"
#include "modelgroupgorka.h"
#include "tos_otcepdata.h"
#include "tos_rc.h"


class m_Zam;

enum{_tos_rc,_tos_dso};

class IGetNewOtcep
{
    public:
    virtual int getNewOtcep(m_RC*rc,int drobl)=0;
    virtual int resetOtcep2prib(int num)=0;
    virtual int nerascep(int num)=0;
};



class tos_System : public BaseWorker
{
    Q_OBJECT
public:
    explicit tos_System(QObject *parent);
    virtual ~tos_System(){}

    void setIGetNewOtcep(IGetNewOtcep *i){iGetNewOtcep=i;}
    tos_OtcepData *getNewOtcep(tos_Rc *trc, int drobl);
    int resetOtcep2prib(int num);
    int nerascep(int num);

    virtual void makeWorkers(ModelGroupGorka *modelGorka);

    QList<SignalDescription> acceptOutputSignals() override;
    virtual void state2buffer() override;
    virtual void work(const QDateTime &T)override;
    virtual void updateOtcepsParams(const QDateTime &T);
    virtual void resetStates()override;
    virtual void resetTracking(int num);
    virtual void resetTracking();

    virtual bool resetDSOBUSY(QString idtsr, QString &acceptStr){Q_UNUSED(idtsr);Q_UNUSED(acceptStr); return false;};


    tos_OtcepData*otcep_by_num(int num);

    ModelGroupGorka *modelGorka=nullptr;

    QList<tos_OtcepData *> lo;
    QList<tos_Rc *> l_trc;
    QMap<m_RC*,m_Zam*> mRc2Zam;
    QMap<m_RC*,m_RIS*> mRc2Ris;
    QMap<m_RC*,m_RC_Gor_ZKR*> mRc2Zkr;
    QMap<m_RC*,m_RC_Gor_Park*> mRc2Park;
    QList<m_RC_Gor_Park *> l_park;
    QList<m_RC_Gor_ZKR *> l_zkr;
    QList<m_Strel_Gor_Y *> l_strel_Y;

    QMap<m_RC*,tos_Rc*> mRc2TRC;
    QMap<int,tos_OtcepData*> mNUM2OD;

signals:
protected:
    IGetNewOtcep *iGetNewOtcep;


};

#endif // TOS_SYSTEM_H
