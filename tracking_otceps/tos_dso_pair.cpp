#include "tos_dso_pair.h"
#include "tos_system_dso.h"

#include "m_strel_gor_y.h"

tos_DsoPair::tos_DsoPair(tos_System_DSO *parent, tos_DSO *tdso1, tos_DSO *tdso0)
{
    //    l_tlg.reserve(64);
//    setObjectName(tdso1->objectName()+" "+tdso0->objectName());
    TOS=parent;
    this->tdso0=tdso0;
    this->tdso1=tdso1;
    strel=qobject_cast<m_Strel_Gor_Y*>(tdso0->dso->rc_next[0]);
    // tdso1 DSO0  <===D0

    rc=TOS->mRc2TRC[tdso0->dso->rc_next[0]];
    rc_next[_back]=TOS->mRc2TRC[tdso0->dso->rc_next[1]];
    rc_next[_forw]=TOS->mRc2TRC[tdso1->dso->rc_next[0]];
}


void tos_DsoPair::resetStates()
{

    sost=_sost_unknow;
    l_os.clear();
}

void tos_DsoPair::work_os(int dso0_os_moved,int dso1_os_moved,TOtcepDataOs os)
{
    // tdso1 tdso0  <===D0


    sost_teleg=_tlg_unknow;


    if ((dso1_os_moved==_os2forw)||(dso0_os_moved==_os2back)){
        if (l_os.isEmpty()) sost=_sost_unknow;
    }
    if  (sost==_sost_unknow) return;

    if  (sost==_sost_wait1t){
        if (dso1_os_moved==_os2forw){
            // вышла
            if (l_os.first().d==_forw) {
                sost=_sost_wait2t;
                d_wait=_forw;
                os_count=l_os.size();
                os_n=1;
            }
            l_os.removeFirst();
        }
        if (dso1_os_moved==_os2back){
            l_os.push_front(os);
        }
        if (dso0_os_moved==_os2forw){
            l_os.push_back(os);
        }
        if (dso0_os_moved==_os2back){
            // вышла
            if (l_os.last().d==_back) {
                sost=_sost_wait2t;
                d_wait=_back;
                os_count=l_os.size();
                os_n=1;
            }
            l_os.removeLast();
        }
        return;
    }
    if  (sost==_sost_wait2t){
        if (dso1_os_moved==_os2forw){
            // вышла
            if (d_wait==_forw){
                os_n++;
                if (os_n==os_count){
                    sost_teleg=_tlg_forw;
                    teleg_os=l_os.first();
                }
                if (os_n==os_count*2){
                    sost=_sost_wait1t;
                    os_count=0;os_n=1;
                    sost_teleg=_tlg_forw;
                    teleg_os=l_os.first();
                }
            }
            l_os.removeFirst();
        }
        if (dso1_os_moved==_os2back){
            l_os.push_front(os);
            if (d_wait==_forw){
                if (os_n>0){
                    l_os.front().d=_forw;
                }
                if (os_n==os_count)  sost_teleg=_tlg_back;
                if (os_n==os_count*2)sost_teleg=_tlg_back;
                os_n--;
                if (os_n==0){
                    sost=_sost_wait1t;
                }
            }
        }
        if (dso0_os_moved==_os2forw){
            l_os.push_back(os);
            if (d_wait==_back){
                if (os_n>0){
                    l_os.last().d=_back;
                }
                if (os_n==os_count)  sost_teleg=_tlg_forw;
                if (os_n==os_count*2)sost_teleg=_tlg_forw;
                os_n--;
                if (os_n==0){
                    sost=_sost_wait1t;
                }
            }

        }
        if (dso0_os_moved==_os2back){
            // вышла
            if (d_wait==_back){
                os_n++;
                if (os_n==os_count){
                    sost_teleg=_tlg_back;
                    teleg_os=l_os.last();
                }
                if (os_n==os_count*2){
                    sost=_sost_wait1t;
                    os_count=0;os_n=1;
                    sost_teleg=_tlg_back;
                    teleg_os=l_os.last();
                }
            }
            l_os.removeLast();
        }
    }

}

void tos_DsoPair::work(const QDateTime &)
{
    // tdso1 tdso0  <===D0

    // сброс при свободности
    if ((rc!=nullptr)&&(rc->rc->STATE_BUSY_DSO()==0)&&
            (rc_next[0]!=nullptr)&&(rc_next[0]->rc->STATE_BUSY_DSO()==0)&&
            (rc_next[1]!=nullptr)&&(rc_next[1]->rc->STATE_BUSY_DSO()==0)
            ){
        sost=_sost_wait1t;
        d_wait=-1;
        os_count=0;
        os_n=0;
        l_os.clear();
    }

    TOtcepDataOs od;
    if (tdso0->os_moved==_os2forw){
        od.d=_forw;
        if ((rc_next[1]!=nullptr) &&  (!rc_next[1]->l_os.isEmpty())) od=rc_next[1]->l_os.first();
        work_os(tdso0->os_moved,_os_none,od);
    }
    if (tdso0->os_moved==_os2back){
        od.d=_back;
        work_os(tdso0->os_moved,_os_none,od);
    }
    if (tdso1->os_moved==_os2forw){
        od.d=_forw;
        work_os(_os_none,tdso1->os_moved,od);
    }
    if (tdso1->os_moved==_os2back){
        od.d=_back;
        if ((rc_next[0]!=nullptr) &&  (!rc_next[0]->l_os.isEmpty())) od=rc_next[0]->l_os.last();
        work_os(_os_none,tdso1->os_moved,od);
    }


}

void tos_DsoPair::free_state()
{
    sost=_sost_wait1t;
    d_wait=-1;
    os_count=0;
    os_n=0;
    l_os.clear();
}

