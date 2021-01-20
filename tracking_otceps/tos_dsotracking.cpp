#include "tos_dsotracking.h"

tos_DsoTracking::tos_DsoTracking(QObject *parent, m_DSO_RD_21 *dso) : BaseWorker(parent)
{
    this->dso=dso;
    setObjectName(dso->objectName());
    dso->disableUpdateStates=true;
    l_statistic.reserve(16);
}

QList<SignalDescription> tos_DsoTracking::acceptOutputSignals()
{
    dso->setSIGNAL_DSODATA(dso->SIGNAL_DSODATA().innerUse());
    QList<SignalDescription> l;
    l << dso->SIGNAL_DSODATA();
    return l;
}

void tos_DsoTracking::state2buffer()
{
    DSO_Data d;
    d.V=dso->STATE_OSY_COUNT();
    d.D=dso->STATE_DIRECT();
    dso->SIGNAL_DSODATA().setValue_data(&d,sizeof (d));
}
void tos_DsoTracking::resetStates()
{
    dso->resetStates();
    current_sost=sost0;
    l_statistic.clear();
    current_sost=sost0;
}

void tos_DsoTracking::work(const QDateTime &T)
{
    struct t_osc21_state_step{
        T_DSO_sost cur_sost;
        int B0;int B1;
        T_DSO_sost next_sost;
        T_DSO_cmd cmd;
    };

    static t_osc21_state_step steps[]=
    {  // sost0    ,    B0, B1 , -> sost2         ,cmd,

       //  взводим
       { sost0,           0 , 0  ,   sostFree,       _none},

       { sostFree,        0 , 0  ,   sostFree,       _none},
       { sostFree,        0 , 1  ,   sostOsEnter01,  _none }, // норм -
       { sostFree,        1 , 0  ,   sostOsEnter10,  _none }, // норм +
       { sostFree,        1 , 1  ,   sostOsEnter11,  _none }, // сбой
       //              ==> INC
       { sostOsEnter10,   0 , 0  ,   sostFree,        _inc_os },// сбой, но занятие было - будем плюсовать
       { sostOsEnter10,   0 , 1  ,   sostOsLeave01,   _none },  // сбой, решаем что проскочили 11
       { sostOsEnter10,   1 , 0  ,   sostOsEnter10,   _none },
       { sostOsEnter10,   1 , 1  ,   sostOsEnter1011, _none },  // норм

       { sostOsEnter1011, 0 , 0  ,   sostFree,        _inc_os }, // сбой, но занятие было - будем плюсовать
       { sostOsEnter1011, 0 , 1  ,   sostOsLeave01,   _none }, // норм
       { sostOsEnter1011, 1 , 0  ,   sostOsEnter10,   _none },
       { sostOsEnter1011, 1 , 1  ,   sostOsEnter1011, _none },

       { sostOsLeave01,   0 , 0  ,   sostFree,         _inc_os }, // норм
       { sostOsLeave01,   0 , 1  ,   sostOsLeave01,    _none },
       { sostOsLeave01,   1 , 0  ,   sostOsEnter10,    _none },  // сбой или поехал обратно? я за сбой
       { sostOsLeave01,   1 , 1  ,   sostOsEnter0111,  _none },  // сбой или поехал обратно? я за сбой

       //             ==>DEC
       { sostOsEnter01,   0 , 0  ,   sostFree,        _inc_os }, // сбой, но занятие было - будем плюсовать, так безопаснее
       { sostOsEnter01,   0 , 1  ,   sostOsEnter01,   _none },
       { sostOsEnter01,   1 , 0  ,   sostOsLeave10,   _none }, //  сбой, решаем что проскочили 11
       { sostOsEnter01,   1 , 1  ,   sostOsEnter0111, _none }, // норм

       { sostOsEnter0111, 0 , 0  ,   sostFree,        _dec_os }, // сбой, но занятие было - будем минусовать
       { sostOsEnter0111, 0 , 1  ,   sostOsEnter01,   _none },
       { sostOsEnter0111, 1 , 0  ,   sostOsLeave10,   _none }, // норм
       { sostOsEnter0111, 1 , 1  ,   sostOsEnter0111, _none },

       { sostOsLeave10,   0 , 0  ,   sostFree,        _dec_os }, // норм
       { sostOsLeave10,   0 , 1  ,   sostOsEnter10,   _none }, // сбой или поехал обратно? я за сбой
       { sostOsLeave10,   1 , 0  ,   sostOsLeave10,   _none },
       { sostOsLeave10,   1 , 1  ,   sostOsEnter0111, _none }, // сбой или поехал обратно? я за сбой

       // непонятки
       { sostOsEnter11,   0 , 0  ,   sostFree,        _inc_os },  // засчитываем за прохождение по +
       { sostOsEnter11,   0 , 1  ,   sostOsLeave01,   _none },  // решаем, что было пропущено занятие 1к
       { sostOsEnter11,   1 , 0  ,   sostOsLeave10,   _none },  // решаем, что было пропущено занятие 1к
       { sostOsEnter11,   1 , 1  ,   sostOsEnter11,   _none }

    };
    if (!dso->is33()){
        int B0=dso->SIGNAL_B0().value_1bit();
        int B1=dso->SIGNAL_B1().value_1bit();
        int K= dso->SIGNAL_K().value_1bit();
        if (K!=1) {
            dso->setSTATE_ERROR(1);
            current_sost=sost0;
        } else {
            // ищем переход
            int cnt=sizeof(steps)/sizeof(steps[0]);
            bool ex=false;
            for (int i=0;i<cnt;i++){
                if ((steps[i].B0==B0)&&
                        (steps[i].B1==B1)&&
                        (steps[i].cur_sost==current_sost)){
                    ex=true;
                    current_sost=steps[i].next_sost;
                    switch (steps[i].cmd) {
                    case _inc_os:
                        inc_os(1,T);
                        break;
                    case _dec_os:
                        inc_os(-1,T);
                        break;
                    default:
                        break;
                    }
                    break;
                }

            }
            if (!ex) current_sost=sost0;
        }
    }
    //    //     запишем результат
    //    qint32 S=0;
    //    qint8 *A=(qint8 *)&S;
    //    A[0]=(qint8) FSTATE_SRAB;
    //    A[1]=(qint8) FSTATE_ERROR;
    //    qint32 OS=FSTATE_OSY_COUNT;

    //    FSIGNAL_STATE_DSO.setData(&S,sizeof(S));
    //    FSIGNAL_STATE_OSY_COUNT.setData(&OS,sizeof(OS));




}





tos_DsoTracking::TDSO_statistic tos_DsoTracking::getStatistic(qlonglong n)
{
    foreach (TDSO_statistic s, l_statistic) {
        if (s.OSY_COUNT==n) {
            return s;
        }
    }
    TDSO_statistic res;
    res.OSY_COUNT=0;
    res.DIRECT=0;
    res.T=QDateTime();
    return res;
}


void tos_DsoTracking::inc_os(int p,QDateTime T)
{
    if (p==0) {
        resetStates();
        return;
    }
    if (p>0){
        dso->setSTATE_OSY_COUNT(dso->STATE_OSY_COUNT()+1);
        dso->setSTATE_DIRECT(0);
    } else
    {
        dso->setSTATE_OSY_COUNT(dso->STATE_OSY_COUNT()-1);
        dso->setSTATE_DIRECT(1);
    }
    TDSO_statistic s;
    s.DIRECT=dso->STATE_DIRECT();
    s.OSY_COUNT=dso->STATE_OSY_COUNT();
    s.T=T;
    l_statistic.push_front(s);
    if (l_statistic.size()>=16) l_statistic.pop_back();
}

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
