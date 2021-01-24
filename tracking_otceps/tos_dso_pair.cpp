#include "tos_dso_pair.h"
tos_DsoPair::tos_DsoPair()
{
    l_tlg.reserve(64);
}

void tos_DsoPair::updateStates(int os1, int os2)
{
    os1=abs(os1);
    os2=abs(os2);
    if ((os1==0)&&(os2==0)) {
        if (sost!=tlg_0){
            c.os_start=0;
            c.tlg_os=0;
            tlg_cnt=0;
            sost=tlg_0;
            l_tlg.clear();
        }
        sost=tlg_0;
    }

    if (sost!=tlg_0){
        if (d==1){
            int os=os1;
            os1=os2;
            os2=os;
        }
    }

    switch (sost) {
    case tlg_error:
        break;
    case tlg_0:{
        if ((os1==0)&&(os2==1)){
            d=0;
            sost=tlg_wait1;
        }
        if ((os1==1)&&(os2==0)){
            d=1;
            sost=tlg_wait1;
        }
    }
        break;
    case tlg_wait1:{
        if ((os1>c.os_start) && (os1==os2)) {
            if (l_tlg.size()>4096){
                sost=tlg_error;
            } else {
                c.tlg_os=os1-c.os_start;
                tlg_cnt++;
                l_tlg.push_back(c);
                c.os_start=os1;
                sost=tlg_wait2;
            }
        } else {
            if ((os1<c.os_start)) {
                if ((os1==c.os_start-1)&&(!l_tlg.isEmpty())){
                    c=l_tlg.last();
                    tlg_cnt--;
                    l_tlg.pop_back();
                    sost=tlg_wait2;
                } else {
                    sost=tlg_error;
                }
            }
        }
    }
        break;
    case tlg_wait2:
    {
        if (os1<c.os_start){
            if ((os1==c.os_start-1)&&(!l_tlg.isEmpty())){
                c=l_tlg.last();
                tlg_cnt--;
                l_tlg.pop_back();
                sost=tlg_wait1;
            } else {
                sost=tlg_error;
            }
        } else if (os1==c.os_start+c.tlg_os){
            tlg_cnt++;
            l_tlg.push_back(c);
            c.os_start=os1;
            c.tlg_os=0;
            sost=tlg_wait1;
        } else if (os1>c.os_start+c.tlg_os){
            sost=tlg_error;
        }
    }
        break;
    } //switch
}

void tos_DsoPair::resetStates()
{
    c.os_start=0;
    c.tlg_os=0;
    tlg_cnt=0;
    sost=tlg_0;
    l_tlg.clear();
}
