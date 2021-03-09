#include "tos_dsotracking.h"

#include "tos_system_dso.h"

tos_DsoTracking::tos_DsoTracking(tos_System_DSO *parent, tos_DSO *dso) : BaseWorker(parent)
{
    this->tdso=dso;
    setObjectName(dso->objectName());
    TOS=parent;

    rc_next[0]=TOS->mRc2TRC[tdso->dso->rc_next[0]];
    rc_next[1]=TOS->mRc2TRC[tdso->dso->rc_next[1]];
    rc_next[0]->tdso[1]=dso;
    rc_next[1]->tdso[0]=dso;
    tdso2=nullptr;
}

void tos_DsoTracking::resetStates()
{

}
void tos_DsoTracking::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;
    tos_DSO *tdso1=tdso;
    if ((tdso->dso->STATE_ERROR()) && (tdso2!=nullptr)) tdso1=tdso2;
    if (tdso1->os_moved==_os2forw)       {
        TOS->moveOs(rc_next[0],rc_next[1],_forw,tdso1->os_v,T);
    }
    if (tdso1->os_moved==_os2back)       {
        TOS->moveOs(rc_next[0],rc_next[1],_back,tdso1->os_v,T);
    }
}

void tos_DsoTracking::add2DSO(tos_DSO *dso)
{
    tdso2=dso;
}


