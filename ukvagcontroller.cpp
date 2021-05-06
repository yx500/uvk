#include "ukvagcontroller.h"
#include "mvp_system.h"
#include "m_otceps.h"
#include "m_otcep.h"
#include "modelgroupgorka.h"

UkVagController::UkVagController(QObject *parent,ModelGroupGorka *GORKA) : BaseWorker(parent)
{
    this->GORKA=GORKA;
    QList<m_Otceps *> l_otceps=GORKA->findChildren<m_Otceps*>();
    this->otceps=l_otceps.first();
    QList<m_UkVag *> l_ukv=GORKA->findChildren<m_UkVag*>();
    ukvag=l_ukv.first();
    if (ukvag->SIGNAL_SET().getBuffer()!=nullptr) ukvag->SIGNAL_SET().getBuffer()->setMsecPeriodLive(3000);
}

void UkVagController::work(const QDateTime &)
{
    int D[3];
    int M[3];
    for (int i=0;i<3;i++) D[i]=0;
    for (int i=0;i<3;i++) M[i]=0;
    if ((GORKA->STATE_REGIM()==ModelGroupGorka::regimRospusk)||(GORKA->STATE_REGIM()==ModelGroupGorka::regimPausa)) {
        int ib=-1;
        for(int i=0;i<otceps->l_otceps.size();i++){
            auto otcep=otceps->l_otceps[i];
            if (!otcep->STATE_ENABLED()) continue;
            if (otcep->STATE_ZKR_PROGRESS()==1) ib=i;
        }
        if (ib<0){
            for(int i=0;i<otceps->l_otceps.size();i++){
                auto otcep=otceps->l_otceps[i];
                if (!otcep->STATE_ENABLED()) continue;
                if (otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) {ib=i;break;}
            }
        }
        if (ib>=0){
            for (int i=0;i<3;i++){
                if (ib+i<otceps->l_otceps.size()){
                    auto otcep=otceps->l_otceps[ib+i];
                    if (!otcep->STATE_ENABLED()) break;
                    D[i]=otcep->STATE_SL_VAGON_CNT();
                    if (D[i]==0) {
                        D[i]=-1;
                    } else {
                        if (otcep->STATE_ZKR_VAGON_CNT()>otcep->STATE_SL_VAGON_CNT()) M[i]=1;
                        if (otcep->STATE_SL_VAGON_CNT_PRED()!=0) M[i]=1;
                    }
                }
            }
        }

        for (int i=0;i<3;i++) {
            if (D[i]>19) D[i]=62;
            else if (D[i]<0) D[i]=45;
            else if (D[i]==0) D[i]=32;
        }
    } else {
        for (int i=0;i<3;i++) D[i]=45;
    }

    ukvag->setSTATE_D1(D[0]);
    ukvag->setSTATE_D2(D[1]);
    ukvag->setSTATE_D3(D[2]);
    ukvag->setSTATE_M1(M[0]);
    ukvag->setSTATE_M2(M[1]);
    ukvag->setSTATE_M3(M[2]);

}

QList<SignalDescription> UkVagController::acceptOutputSignals()
{
    QList<SignalDescription> l;
    ukvag->setSIGNAL_DATA(ukvag->SIGNAL_DATA().innerUse()); //l<<ukvag->SIGNAL_DATA();
    ukvag->setSIGNAL_SET(ukvag->SIGNAL_SET().innerUse()); l<<ukvag->SIGNAL_SET();
    return l;
}

void UkVagController::state2buffer()
{
    UkVag ukv;
    memset(&ukv,0,sizeof(ukv));
    if ((setLightTimer.isValid()) &&(setLightTimer.elapsed()<3000)){
        for (int i=0;i<3;i++){
            ukv.data[i].D=255;
            ukv.data[i].D8[0]=0x81;
            ukv.data[i].D8[1]=1;
            ukv.data[i].D8[2]=light;
        }
    } else {
        ukv.data[0].D=ukvag->STATE_D1();
        ukv.data[1].D=ukvag->STATE_D2();
        ukv.data[2].D=ukvag->STATE_D3();
        ukv.data[0].M=ukvag->STATE_M1();
        ukv.data[1].M=ukvag->STATE_M2();
        ukv.data[2].M=ukvag->STATE_M3();
    }
    ukvag->SIGNAL_SET().setBufferData(&ukv,sizeof(ukv));
}

void UkVagController::setLight(int d)
{
    if ((d>=1)&&(d<=7)) light=d;
    setLightTimer.start();
}
