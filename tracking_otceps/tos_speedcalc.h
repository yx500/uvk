#ifndef TOS_SPEEDCALC_H
#define TOS_SPEEDCALC_H

#include "m_base.h"
#include "m_otcep.h"
#include "tos_otcepdata.h"


// Набор функций для расчета скорости



class tos_SpeedCalc
{
public:
    static qreal calcV(QDateTime t1,QDateTime t2,qreal len_m);
    static qreal calcV(qint64 ms, qreal len_m);
    static qreal vt_len_m(qreal V,qint64 ms);
    static qint64 calcMs(qreal V, qreal len_m);

    //virtual void work(const QDateTime &T);
    //void otcepMoved(m_Otcep *otcep, m_RC *rcsFrom, m_RC *rcsTo, m_RC *rcfFrom, m_RC *rcfTo);
protected:
    //m_Otceps * otceps;

};

#endif // TOS_SPEEDCALC_H
