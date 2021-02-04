#include "tos_dso_pair.h"
#include "m_strel_gor_y.h"
tos_DsoPair::tos_DsoPair(tos_System_DSO *parent, tos_DSO *dso1, tos_DSO *dso2) : tos_DsoTracking(parent,dso2)
{
    //    l_tlg.reserve(64);
    setObjectName(dso1->objectName()+" "+dso2->objectName());
    this->dso1=dso1;
    this->dso2=dso2;
    strel=qobject_cast<m_Strel_Gor_Y*>(dso1->dso->rc);
}

//void tos_DsoPair::updateStates_n(int os1, int os2)
//{
//    os1=abs(os1);
//    os2=abs(os2);
//    if ((os1==0)&&(os2==0)) {
//        if (sost!=tlg_0){
//            c.os_start=0;
//            c.tlg_os=0;
//            tlg_cnt=0;
//            sost=tlg_0;
//            l_tlg.clear();
//        }
//        sost=tlg_0;
//    }

//    if (sost!=tlg_0){
//        if (d==1){
//            int os=os1;
//            os1=os2;
//            os2=os;
//        }
//    }

//    switch (sost) {
//    case tlg_error:
//        break;
//    case tlg_0:{
//        if ((os1==0)&&(os2==1)){
//            d=0;
//            sost=tlg_wait1;
//        }
//        if ((os1==1)&&(os2==0)){
//            d=1;
//            sost=tlg_wait1;
//        }
//    }
//        break;
//    case tlg_wait1:{
//        if ((os1>c.os_start) && (os1==os2)) {
//            if (l_tlg.size()>4096){
//                sost=tlg_error;
//            } else {
//                c.tlg_os=os1-c.os_start;
//                tlg_cnt++;
//                l_tlg.push_back(c);
//                c.os_start=os1;
//                sost=tlg_wait2;
//            }
//        } else {
//            if ((os1<c.os_start)) {
//                if ((os1==c.os_start-1)&&(!l_tlg.isEmpty())){
//                    c=l_tlg.last();
//                    tlg_cnt--;
//                    l_tlg.pop_back();
//                    sost=tlg_wait2;
//                } else {
//                    sost=tlg_error;
//                }
//            }
//        }
//    }
//        break;
//    case tlg_wait2:
//    {
//        if (os1<c.os_start){
//            if ((os1==c.os_start-1)&&(!l_tlg.isEmpty())){
//                c=l_tlg.last();
//                tlg_cnt--;
//                l_tlg.pop_back();
//                sost=tlg_wait1;
//            } else {
//                sost=tlg_error;
//            }
//        } else if (os1==c.os_start+c.tlg_os){
//            tlg_cnt++;
//            l_tlg.push_back(c);
//            c.os_start=os1;
//            c.tlg_os=0;
//            sost=tlg_wait1;
//        } else if (os1>c.os_start+c.tlg_os){
//            sost=tlg_error;
//        }
//    }
//        break;
//    } //switch
//}

void tos_DsoPair::resetStates()
{
    //    c.os_start=0;
    //    c.tlg_os=0;
    //    tlg_cnt=0;
    //    sost=tlg_0;
    //    l_tlg.clear();
    sost=_sost_unknow;
    l_os.clear();
}
void tos_DsoPair::work(const QDateTime &)
{
    // DSO1 DSO2  <===D0

    // сброс при свободности
    if ((dso1->dso->rc!=nullptr)&&(dso1->dso->rc->STATE_BUSY_DSO()==0)&&
            (dso1->dso->rc->next_rc[0]!=nullptr)&&(dso1->dso->rc->next_rc[0]->STATE_BUSY_DSO()==0)&&
            (dso1->dso->rc->next_rc[1]!=nullptr)&&(dso1->dso->rc->next_rc[1]->STATE_BUSY_DSO()==0)
            ){
        sost=_sost_wait1t;
        d_wait=-1;
        os_count=0;
        os_n=0;
        l_os.clear();
    }

    sost_teleg=_tlg_unknow;


    if ((dso1->os_moved==_os2forw)||(dso2->os_moved==_os2back)){
        if (l_os.isEmpty()) sost=_sost_unknow;
    }
    if  (sost==_sost_unknow) return;

    if  (sost==_sost_wait1t){
        if (dso1->os_moved==_os2forw){
            // вышла
            if (l_os.first().d==_forw) {
                sost=_sost_wait2t;
                d_wait=_forw;
                os_count=l_os.size();
                os_n=1;
            }
            l_os.removeFirst();
        }
        if (dso1->os_moved==_os2back){
            add_os(_back);
        }
        if (dso2->os_moved==_os2forw){
            add_os(_forw);
        }
        if (dso2->os_moved==_os2back){
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
        if (dso1->os_moved==_os2forw){
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
        if (dso1->os_moved==_os2back){
            add_os(_back);
            if (d_wait==_forw){
                if (os_n>0){
                    l_os.front().d=_forw;
                }
                os_n--;
                if (os_n==0){
                    sost=_sost_wait1t;
                }
            }
        }
        if (dso2->os_moved==_os2forw){
            add_os(_forw);
            if (d_wait==_back){
                if (os_n>0){
                    l_os.last().d=_back;
                }
                os_n--;
                if (os_n==0){
                    sost=_sost_wait1t;
                }
            }

        }
        if (dso2->os_moved==_os2back){
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

void tos_DsoPair::add_os(int d)
{
    TOtcepDataOs od;od.d=d;
    if (d==_forw)  {
        if ((rc_next[1]!=nullptr) &&  (!rc_next[1]->l_os.isEmpty())){
            od=rc_next[1]->l_os.first();
            od.d=d;
        }
        l_os.push_back(od);
    }
    if (d==_back)  l_os.push_front(od);

}
