#include "tos_dsotracking.h"

#include "tos_system_dso.h"

tos_DsoTracking::tos_DsoTracking(tos_System_DSO *parent, tos_DSO *dso) : BaseWorker(parent)
{
    this->tdso=dso;
    setObjectName(dso->objectName());
    TOS=parent;

    rc_next[0]=TOS->mRc2TRC[tdso->dso->rc_next[0]];
    rc_next[1]=TOS->mRc2TRC[tdso->dso->rc_next[1]];
    rc_next[0]->tdso[0]=dso;
    rc_next[1]->tdso[1]=dso;
}

void tos_DsoTracking::resetStates()
{

}
void tos_DsoTracking::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;
    if (tdso->os_moved==_os2forw)       {
        TOS->moveOs(rc_next[0],rc_next[1],_forw,tdso->os_v,T);
    }
    if (tdso->os_moved==_os2back)       {
        TOS->moveOs(rc_next[0],rc_next[1],_back,tdso->os_v,T);
    }
}



