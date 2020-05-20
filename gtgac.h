#ifndef GTGAC_H
#define GTGAC_H

#include "baseworker.h"
#include "trackingotcepsystem.h"
#include "m_strel_gor_y.h"



class GtGac : public BaseWorker
{
        Q_OBJECT
public:
    explicit GtGac(QObject *parent,TrackingOtcepSystem *TOS);
    virtual ~GtGac()override{}
    void resetStates() override;
    void work(const QDateTime &T) override;
    void validation(ListObjStr *l) const override;
    void sendCommand(m_Strel_Gor_Y *strel, MVP_Enums::TStrelPol pol_cmd, bool force=false);
    void state2buffer()override;
signals:
    void uvk_command(const SignalDescription &s,int state);
protected:
    TrackingOtcepSystem *TOS;
    QList<m_Strel_Gor_Y*> l_strel;
    static bool inway(int way,int minway,int maxway)
    {
        if ((way>0)&&(way>=minway)&&(way<=maxway)) return true;
        return false;
    }
};

#endif // GTGAC_H
