#include "tos_speedcalc.h"
#include "m_otceps.h"
#include "m_rc_gor_zam.h"
#include "m_rc_gor_zkr.h"
#include <math.h>


qreal tos_SpeedCalc::calcV(QDateTime t1, QDateTime t2, qreal len)
{
    qint64 ms=t1.msecsTo(t2);
    if (ms!=0) return 3600.*len/ms;
    return 0;
}

qreal tos_SpeedCalc::calcV(qint64 ms, qreal len)
{
    if (ms==0) return 0;
    return 3600.*len/ms;
}





//static qreal xtv_len(QList<TTXV> &l,QDateTime T=QDateTime())
//{
//    qint64 ms;
//    if (l.isEmpty()) return 0;
//    TTXV tvx=l.first();
//    qreal xx=tvx.x;
//    for (int i=1;i<l.size();i++){
//        const TTXV &tvx1=l.at(i);
//        ms=tvx.t.msecsTo(tvx1.t);
//        xx=xx+tvx.v*(1.*ms/3600.);
//        tvx=tvx1;
//    }
//    if (T.isValid()){
//        ms=tvx.t.msecsTo(T);
//        xx=xx+tvx.v*(1.*ms/3600.);
//    }
//    return xx;
//}

qreal tos_SpeedCalc::vt_len(qreal V,qint64 ms)
{
    return   V*(1.*ms/3600.);
}



//void calc_xv(QList<t_txv> &l_txv, qreal maxlen)
//{
//    if (l_txv.isEmpty()) return;
//    qint64 ms;
//    t_txv txv=l_txv.first();
//    qreal xx=txv.x;
//    for (int i=1;i<l_txv.size();i++){
//        t_txv &txv1=l_txv[i];
//        ms=txv.t.msecsTo(txv1.t);
//        xx=xx+txv.v*(1.*ms/3600.);
//        txv=txv1;
//        txv1.x=xx;

//    }
//    if (xx>maxlen) {
//        qreal kx=xx/maxlen;
//        for (int i=1;i<l_txv.size();i++){
//            t_txv &txv1=l_txv[i];
//            txv1.x=txv1.x*kx;
//        }
//    }
//}



//int pred_txv(QList<t_txv> &l_txv,int ii,qreal len)
//{
//    const t_txv &txv0=l_txv.at(ii);
//    for (int i=ii;i>=0;i--){
//        const t_txv &txv=l_txv.at(i);
//        if ((txv0.x-txv.x)<=len) continue;
//        return i;
//    }
//    return 0;
//}

//void calc_vx(qreal v0,QList<t_txv> &l_txv, qreal x_step)
//{
//    if (l_txv.isEmpty()) return;
//    for (int i=0;i<l_txv.size();i++){
//        t_txv &txv1=l_txv[i];
//        if (txv1.x<x_step){
//            txv1.v=v0;
//            continue;
//        }
//        int i2=pred_txv(l_txv,i,x_step);
//        const t_txv &txv2=l_txv[i2];
//        qreal r=txv1.x-txv2.x;
//        txv1.v=tos_SpeedCalc::calcV(txv2.t,txv1.t,r);
//    }
//}
