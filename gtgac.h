#ifndef GTGAC_H
#define GTGAC_H
#include <QElapsedTimer>
#include "modelgroupgorka.h"
#include "baseworker.h"
#include "m_strel_gor_y.h"


struct GacStrel{
    m_Strel_Gor_Y* strel;
    // ГАЦ
    MVP_Enums::TStrelPol pol_zad;
    MVP_Enums::TStrelPol pol_cmd;
    MVP_Enums::TStrelPol pol_mar;
    MVP_Enums::TStrelPol pol_av;
    QElapsedTimer pol_cmd_time;
    QDateTime pol_cmd_w_time;
    QElapsedTimer emit_time;
    QDateTime sred_pol_time;
    bool ERR_PLATA;
    bool ERR_PER;
    bool BL_PER_DONE;
    MVP_Enums::TStrelPol pol_UVK_BL_PER_DONE;

    bool BL_PER;
    bool BL_PER_SP;
    bool BL_PER_OTC;
    bool BL_PER_TLG;
    bool BL_PER_DB;
    bool BL_PER_NGBSTAT;
    bool BL_PER_NGBDYN;
    bool BL_SRPOL_TO;


    SignalDescription SIGNAL_UVK_ERR_PLATA;
    SignalDescription SIGNAL_UVK_ERR_PER;
    SignalDescription SIGNAL_UVK_PRP_MAR;
    SignalDescription SIGNAL_UVK_PRM_MAR;
    SignalDescription SIGNAL_UVK_BL_PER_DONE;


    SignalDescription SIGNAL_UVK_BL_PER_SP;
    SignalDescription SIGNAL_UVK_BL_PER_DB;
    SignalDescription SIGNAL_UVK_BL_PER_OTC;
    SignalDescription SIGNAL_UVK_BL_PER_TLG;
    SignalDescription SIGNAL_UVK_BL_PER_NGBSTAT;
    SignalDescription SIGNAL_UVK_BL_PER_NGBDYN;
    SignalDescription SIGNAL_UVK_BL_SRPOL_TO;





};




class GtGac : public BaseWorker
{
        Q_OBJECT
public:
    explicit GtGac(QObject *parent,ModelGroupGorka *modelGorka);
    virtual ~GtGac()override{}
    void resetStates() override;
    void work(const QDateTime &T) override;
    void validation(ListObjStr *l) const override;
    void sendCommand(GacStrel *gs, MVP_Enums::TStrelPol pol_cmd, bool force=false);
    void state2buffer()override;
    QList<SignalDescription> acceptOutputSignals() override;
    void setStateBlockPerevod(GacStrel *gs,const QDateTime &T);
    QList<GacStrel*> l_strel;

    bool PER_NGB_enabled=false;

signals:
    void uvk_command(const SignalDescription &s,int state);
protected:
    ModelGroupGorka *modelGorka;
    m_Otceps *otceps;

    QList<m_RC_Gor_ZKR*> l_zkr;
    QMap<m_RC *,GacStrel*> mRC2GS;

    void set_STATE_GAC_ACTIVE();


};

#endif // GTGAC_H
