#ifndef TOS_SPEEDCALC_H
#define TOS_SPEEDCALC_H

#include "m_base.h"
#include "m_otcep.h"
#include "tos_otcepdata.h"


// Набор функций для расчета скорости



class tos_SpeedCalc
{
public:
    static qreal calcV(QDateTime t1,QDateTime t2,qreal len);
    static qreal calcV(qint64 ms,qreal len);
    static qreal vt_len(qreal V,qint64 ms);

    //virtual void work(const QDateTime &T);
    //void otcepMoved(m_Otcep *otcep, m_RC *rcsFrom, m_RC *rcsTo, m_RC *rcfFrom, m_RC *rcfTo);
protected:
    //m_Otceps * otceps;

};

#endif // TOS_SPEEDCALC_H
