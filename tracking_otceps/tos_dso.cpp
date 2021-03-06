#include "tos_dso.h"

tos_DSO::tos_DSO(QObject *parent, m_DSO_RD_21 *dso) : BaseWorker(parent)
{
    this->dso=dso;
    setObjectName(dso->objectName());
    dso->disableUpdateStates=true;
    l_statistic.reserve(16);
}

QList<SignalDescription> tos_DSO::acceptOutputSignals()
{
    dso->setSIGNAL_DSODATA(dso->SIGNAL_DSODATA().innerUse());
    QList<SignalDescription> l;
    l << dso->SIGNAL_DSODATA();
    dso->SIGNAL_DSODATA().getBuffer()->setSizeData(DSO_Data_Max*sizeof(t_OsyCell_21));
    return l;
}

void tos_DSO::state2buffer()
{
    t_OsyCell_21 d;
    d.V=dso->STATE_OSY_COUNT();
    d.D=dso->STATE_DIRECT();
    d.E=dso->STATE_ERROR_TRACK();
    d.EV=dso->STATE_ERROR_CNT();
    d.Vel=os_v*10;
    if (os_v==_undefV_)d.Vel=0;
    dso->SIGNAL_DSODATA().setValue_data(&d,sizeof (d));
}

void tos_DSO::resetTracking()
{
    dso->setSTATE_OSY_COUNT(0);
    dso->setSTATE_ERROR_TRACK(false);
    //dso->setSTATE_ERROR_CNT(0);
}

void tos_DSO::resetStates()
{
    dso->resetStates();
    current_sost=sost0;
    l_statistic.clear();
    current_sost=sost0;
    os_moved=_os_none;
}

void tos_DSO::work(const QDateTime &T)
{
    struct t_osc21_state_step{
        T_DSO_sost cur_sost;
        int B0;int B1;
        T_DSO_sost next_sost;
        T_DSO_cmd cmd;
    };
    auto wdso=alive_dso();
    static t_osc21_state_step steps[]=
    {  // sost0    ,    B0, B1 , -> sost2         ,cmd,

       //  взводим
       { sost0,           0 , 0  ,   sostFree,       _none},

       { sostFree,        0 , 0  ,   sostFree,       _none},
       { sostFree,        0 , 1  ,   sostOsEnter01,  _none }, // норм -
       { sostFree,        1 , 0  ,   sostOsEnter10,  _none }, // норм +
       { sostFree,        1 , 1  ,   sostOsEnter11,  _sboy }, // сбой
       //              ==> INC
       { sostOsEnter10,   0 , 0  ,   sostFree,        _none },// сбой, но занятие было - будем плюсовать
       { sostOsEnter10,   0 , 1  ,   sostOsLeave01,   _sboy },  // сбой, решаем что проскочили 11
       { sostOsEnter10,   1 , 0  ,   sostOsEnter10,   _none },
       { sostOsEnter10,   1 , 1  ,   sostOsEnter1011, _none },  // норм

       { sostOsEnter1011, 0 , 0  ,   sostFree,        _inc_os_sboy }, // сбой, но занятие было - будем плюсовать
       { sostOsEnter1011, 0 , 1  ,   sostOsLeave01,   _none }, // норм
       { sostOsEnter1011, 1 , 0  ,   sostOsEnter10,   _none },
       { sostOsEnter1011, 1 , 1  ,   sostOsEnter1011, _none },

       { sostOsLeave01,   0 , 0  ,   sostFree,         _inc_os }, // норм
       { sostOsLeave01,   0 , 1  ,   sostOsLeave01,    _none },
       { sostOsLeave01,   1 , 0  ,   sostOsEnter10,    _sboy },  // сбой или поехал обратно? я за сбой
       { sostOsLeave01,   1 , 1  ,   sostOsEnter1011,  _none },  //  поехал обратно? я за сбой

       //             <==DEC
       { sostOsEnter01,   0 , 0  ,   sostFree,        _none }, // сбой, но занятие было - будем плюсовать, так безопаснее
       { sostOsEnter01,   0 , 1  ,   sostOsEnter01,   _none },
       { sostOsEnter01,   1 , 0  ,   sostOsLeave10,   _sboy }, //  сбой, решаем что проскочили 11
       { sostOsEnter01,   1 , 1  ,   sostOsEnter0111, _none }, // норм

       { sostOsEnter0111, 0 , 0  ,   sostFree,        _dec_os_sboy }, // сбой, но занятие было - будем минусовать
       { sostOsEnter0111, 0 , 1  ,   sostOsEnter01,   _none },
       { sostOsEnter0111, 1 , 0  ,   sostOsLeave10,   _none }, // норм
       { sostOsEnter0111, 1 , 1  ,   sostOsEnter0111, _none },

       { sostOsLeave10,   0 , 0  ,   sostFree,        _dec_os }, // норм
       { sostOsLeave10,   0 , 1  ,   sostOsEnter01,   _sboy }, // сбой или поехал обратно? я за сбой
       { sostOsLeave10,   1 , 0  ,   sostOsLeave10,   _none },
       { sostOsLeave10,   1 , 1  ,   sostOsEnter0111, _none }, // поехал обратно

       // непонятки
       { sostOsEnter11,   0 , 0  ,   sostFree,        _sboy_last_d },  // плюсуем по текущему напр
       { sostOsEnter11,   0 , 1  ,   sostOsLeave01,   _none },  // решаем, что было пропущено занятие 1к
       { sostOsEnter11,   1 , 0  ,   sostOsLeave10,   _none },  // решаем, что было пропущено занятие 1к
       { sostOsEnter11,   1 , 1  ,   sostOsEnter11,   _none }

    };

    os_moved=_os_none;
    if (!wdso->is33()){
        int B0=wdso->SIGNAL_B0().value_1bit();
        int B1=wdso->SIGNAL_B1().value_1bit();
        if (B0==0) B0=1; else B0=0;
        if (B1==0) B1=1; else B1=0;
        int K= wdso->SIGNAL_K().value_1bit();
        if (K!=1) {
            //            dso->setSTATE_ERROR(1);
            //            current_sost=sost0;
        }
        {

            // ищем переход
            int cnt=sizeof(steps)/sizeof(steps[0]);
            bool ex=false;
            for (int i=0;i<cnt;i++){
                if ((steps[i].B0==B0)&&
                        (steps[i].B1==B1)&&
                        (steps[i].cur_sost==current_sost)){
                    ex=true;
                    if (current_sost!=steps[i].next_sost){
                        current_sost=steps[i].next_sost;
                        switch (steps[i].cmd) {
                        case _inc_os:
                            wdso->setSTATE_ERROR_TRACK(0);
                            inc_os(1,T);
                            break;
                        case _dec_os:
                            wdso->setSTATE_ERROR_TRACK(0);
                            inc_os(-1,T);
                            break;
                        case _inc_os_sboy:
                            wdso->setSTATE_ERROR_TRACK(1);
                            wdso->setSTATE_ERROR_CNT(wdso->STATE_ERROR_CNT()+1);
                            inc_os(1,T);
                            break;
                        case _dec_os_sboy:
                            wdso->setSTATE_ERROR_TRACK(1);
                            wdso->setSTATE_ERROR_CNT(wdso->STATE_ERROR_CNT()+1);
                            inc_os(-1,T);
                            break;
                        case _sboy_last_d:
                            wdso->setSTATE_ERROR_TRACK(1);
                            wdso->setSTATE_ERROR_CNT(wdso->STATE_ERROR_CNT()+1);
                            if (wdso->STATE_DIRECT()==0) inc_os( 1,T); else
                                inc_os(-1,T);
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                }

            }
            if (!ex) current_sost=sost0;
        }
    }

}





tos_DSO::TDSO_statistic tos_DSO::getStatistic(qlonglong n)
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

m_DSO_RD_21 *tos_DSO::alive_dso()
{

    if (dso->STATE_ERROR()){
        if ((dso->dso_dubl!=nullptr) && (!dso->dso_dubl->STATE_ERROR())) return qobject_cast<m_DSO_RD_21 *>(dso->dso_dubl);
    }
    return dso;
}


void tos_DSO::inc_os(int p,QDateTime T)
{
    if (p==0) {
        resetStates();
        return;
    }
    if (p>0){
        dso->setSTATE_OSY_COUNT(dso->STATE_OSY_COUNT()+1);
        dso->setSTATE_DIRECT(0);
        os_moved=_os2forw;
    } else
    {
        dso->setSTATE_OSY_COUNT(dso->STATE_OSY_COUNT()-1);
        dso->setSTATE_DIRECT(1);
        os_moved=_os2back;
    }

    TDSO_statistic s;
    s.DIRECT=dso->STATE_DIRECT();
    s.OSY_COUNT=dso->STATE_OSY_COUNT();
    s.T=T;

    //скорость по расстоянию тележуки
    os_v=_undefV_;
    if (!l_statistic.isEmpty()){
        TDSO_statistic s1=l_statistic.first();
        if (s1.DIRECT==s.DIRECT){
            if (s.OSY_COUNT%2==0){
                auto ms=s1.T.msecsTo(s.T);
                static qreal k=1850.*3.6;
                if (ms>0) os_v=k/ms;
            }
        }
    }

    l_statistic.push_front(s);
    if (l_statistic.size()>=16) l_statistic.pop_back();
}

