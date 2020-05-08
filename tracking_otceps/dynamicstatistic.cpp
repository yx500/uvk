#include "dynamicstatistic.h"
#include "tos_speedcalc.h"
#include "m_rc_gor_zkr.h"
#include "m_rc_gor_park.h"
#include "m_ris.h"
#include <qfileinfo.h>
#include <QtMath>

const  qreal _dv_category=1.;
const  int _max_rec_in_dv_category=10;


bool stat_dataTLessThan(const t_rc_stat_data &d1, const t_rc_stat_data &d2)
{
    if (d1.T<d2.T) return true;
    return false;
}

DynamicStatistic::DynamicStatistic(QObject *parent, TrackingOtcepSystem *TOS) : BaseWorker(parent)
{
    this->TOS=TOS;
    connect(TOS,&TrackingOtcepSystem::otcep_rcsf_change,this,&DynamicStatistic::slot_otcep_rcsf_change);
    auto l_rc=TOS->modelGorka->findChildren<m_RC*>();
    foreach (auto rc, l_rc) {
        mRc2Stat.insert(rc->id(),rc_stat());
        // нйдем нужный тп
        m_RC_Gor_Zam * zam=qobject_cast<m_RC_Gor_Zam *>(rc);
        m_RC*rc1=rc;
        int ntp=0;
        while (zam!=nullptr){
            rc1=zam->getNextRC(1,0);
            zam=qobject_cast<m_RC_Gor_Zam *>(rc1);
        }

        while (rc1!=nullptr) {
            zam=qobject_cast<m_RC_Gor_Zam *>(rc1);
            if (zam!=nullptr) {ntp=zam->zam()->NTP();break;}
            m_RC_Gor*rcn=qobject_cast<m_RC_Gor*>(rc1->getNextRC(1,0));
            if ((rcn!=nullptr)&&(rcn->MINWAY()>0)) rc1=rcn; else rc1=qobject_cast<m_RC_Gor*>(rc1->getNextRC(1,1));
        }
        mRc2NTP[rc]=ntp;
    }
    l_rc_park=TOS->modelGorka->findChildren<m_RC_Gor_Park*>();
    makeDefault();
    load_stat("./dyn_stat.bin");

}

bool DynamicStatistic::load_stat(QString fn)
{
    mRc2Stat.clear();
    QFileInfo fi(fn);
    if (fi.exists()){
        QFile F;
        F.setFileName(fn);
        if (F.open(QFile::ReadOnly)){
            if (F.seek(0)){
                while (!F.atEnd()){
                    t_rc_stat_data d;
                    if (F.read(( char *)&d,sizeof(d))==sizeof(d)){
                        if (!mRc2Stat.contains(d.id)){
                            mRc2Stat.insert(d.id,rc_stat());
                        }
                        rc_stat &stat=mRc2Stat[d.id];
                        stat.l.push_back(d);
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool DynamicStatistic::write_stat(QString fn)
{
    QFile F;
    F.setFileName(fn);
    if (F.open(QFile::WriteOnly)){
        F.seek(0);
        foreach (auto id, mRc2Stat.keys()) {
            rc_stat &stat=mRc2Stat[id];
            foreach (const t_rc_stat_data &d, stat.l) {
                F.write(( char *)&d,sizeof(d));
            }
        }
        return true;
    }
    return false;
}


qreal DynamicStatistic::defaultVprognoz(t_rc_stat_data categ_d, m_RC *rc)
{
    m_RC_Gor_Zam *rc_zam=qobject_cast<m_RC_Gor_Zam *>(rc);

    m_RC_Gor_Park *rc_park=qobject_cast<m_RC_Gor_Park *>(rc);
    if (rc_park){
        return categ_d.v_in-(1.*categ_d.v_in*categ_d.x1/rc_park->LEN())+categ_d.category_ves();;
    }
    return categ_d.v_in+categ_d.category_ves();

}


QList<t_rc_stat_data> DynamicStatistic::rc_marshrut(int put_nadvig, int m)
{
    QList<t_rc_stat_data> ld;
    t_rc_stat_data d;

    QList<m_RC_Gor *> lm=TOS->modelGorka->marshrut(put_nadvig,m);
    foreach (m_RC_Gor *rc, lm) {
        d.marf=m;
        d.id=rc->id();
        d.absx=rc->m_PN_M2X[put_nadvig][m];
        d._rc=rc;
        d.x1=0;
        d.x2=rc->LEN();

        m_RC_Gor_Zam *rc_zam=qobject_cast<m_RC_Gor_Zam *>(rc);
        if (rc_zam!=nullptr) {
            d.id_tp=rc_zam->id();
        }

        m_RC_Gor_Park *rc_park=qobject_cast<m_RC_Gor_Park *>(rc);
        if (rc_park){
            qreal dx=_ds_parck_div;
            int n=rc_park->LEN()/dx+1;
            for (int i=0;i<n;i++){
                d.x1=i*dx;
                d.x2=(i+1)*dx;
                d.absx=rc->m_PN_M2X[put_nadvig][m]+d.x1;
                ld.push_back(d);
            }
        }else {
            ld.push_back(d);
        }
    }
    return ld;
}
void DynamicStatistic::makeDefault()
{
    QList<t_rc_stat_data> ld;
    t_rc_stat_data d;


    QList<m_RC_Gor_ZKR*> lzkr=TOS->modelGorka->findChildren<m_RC_Gor_ZKR*>();
    m_RC_Gor_ZKR *zkr=lzkr.first();
    for (int m=zkr->MINWAY();m<=zkr->MAXWAY();m++){
        QList<t_rc_stat_data> ldx=rc_marshrut(zkr->PUTNADVIG(),m);
        for (int ives=0;ives<4;ives++){
            if (ives==0) d.ves=70;
            if (ives==1) d.ves=120;
            if (ives==2) d.ves=150;
            if (ives==3) d.ves=300;
            for (int ivag=0;ivag<4;ivag++){
                if (ivag==0) d.vag=1;
                if (ivag==1) d.vag=2;
                if (ivag==2) d.vag=4;
                if (ivag==3) d.vag=6;

                for (int vv=0;vv<2;vv++){
                    d.v_out_tp=0;
                    d.ms_out_tp=0;
                    foreach (t_rc_stat_data dx, ldx) {
                        d.id=dx.id;
                        d.x1=dx.x1;
                        d.x2=dx.x2;
                        d.marf=dx.marf;
                        d._rc=dx._rc;
                        d.absx=dx.absx;
                        m_RC_Gor_Zam *rc_zam=qobject_cast<m_RC_Gor_Zam *>(dx._rc);
                        if (rc_zam!=nullptr) {
                            d.ms_out_tp=0;
                            d.v_out_tp=5.+vv*5;
                            d.v_in=d.v_out_tp;
                            d.v_out=d.v_out_tp;
                            d.ms=(d.x2-d.x1)*3600/((d.v_in+d.v_out_tp)/2);
                            add_stat_data(d,_default);
                            continue;
                        }
                        m_RC_Gor_Park *rc_park=qobject_cast<m_RC_Gor_Park *>(dx._rc);
                        if (rc_park){
                            d.v_in=d.v_out_tp*(1.-d.x1/rc_park->LEN());
                            d.v_out=d.v_out_tp*(1.-d.x2/rc_park->LEN());
                        }
                        d.ms=(d.x2-d.x1)*3600/((d.v_in+d.v_out_tp)/2);
                        add_stat_data(d,_default);
                        d.ms_out_tp=d.ms_out_tp+d.ms;
                    }
                }
            }
        }
    }
}



void DynamicStatistic::slot_otcep_rcsf_change(m_Otcep *otcep,int sf, m_RC *rcFrom, m_RC *rcTo, QDateTime T, QDateTime Trc)
{
    if (sf==1){
        qint64 ms=Trc.msecsTo(T);
        qreal v_out=_undefV_;
        qreal v_in=otcep->tos->STATE_V_RCF_IN();
        if ((rcTo!=nullptr)&&(rcFrom!=nullptr)&&(ms>0)){
            if  (rcFrom==otcep->tos->out_tp_rc){
                v_out=otcep->tos->out_tp_v;
            } else {
                v_out=tos_SpeedCalc::calcV(ms,rcFrom->LEN());
            }
            if ((otcep->STATE_V_RC()!=_undefV_)&&(rcTo)){
                if ((rcFrom!=nullptr) && (Trc.isValid())) add_stat(T,otcep,rcFrom,0,rcFrom->LEN(),ms,v_in,v_out);
            }
        }
        otcep->tos->setSTATE_V_RCF_IN(v_out);
    }

}

void DynamicStatistic::add_stat(const QDateTime &T, m_Otcep *otcep,  m_RC *rc,int x1, int x2, qint64 ms,qreal v_in,qreal v_out)
{

    t_rc_stat_data d;
    d.id=rc->id();
    m_RC_Gor* rc_gor=qobject_cast<m_RC_Gor*>(rc);
    d.absx=rc_gor->m_PN_M2X[otcep->STATE_PUT_NADVIG()][otcep->STATE_MAR()]+x1;

    d.T=T;
    d.ves=otcep->STATE_VES();
    d.vag=otcep->STATE_VAGON_CNT();
    d.marf=otcep->STATE_MAR();
    if ((otcep->STATE_V_OUT_1()!=_undefV_) && (otcep->STATE_V_IN_2()!=_undefV_)&&(qFabs(otcep->STATE_V_OUT_1()-otcep->STATE_V_IN_2())<10))
        d.dV12=otcep->STATE_V_OUT_1()-otcep->STATE_V_IN_2(); else d.dV12=_undefV_;
    if ((otcep->STATE_V_OUT_2()!=_undefV_) && (otcep->STATE_V_IN_3()!=_undefV_)&&(qFabs(otcep->STATE_V_OUT_2()-otcep->STATE_V_IN_3())<10))
        d.dV23=otcep->STATE_V_OUT_2()-otcep->STATE_V_IN_3(); else d.dV23=_undefV_;
    //    int ntp=mRc2NTP[rc];


    //    if (ntp>0){
    //        if (ntp==1) {d.v_in=otcep->STATE_V_OUT_1();d.dV12=0;d.dV23=0;}
    //        if (ntp==2) {d.v_in=otcep->STATE_V_OUT_2();d.dV23=0;}
    //        if (ntp==3) d.v_in=otcep->STATE_V_OUT_3();
    //        if (otcep->tos->V_OUT_T[ntp-1].isValid()) d.ms_out_tp=otcep->tos->V_OUT_T[ntp-1].msecsTo(T);
    //        if (otcep->tos->_rc_zam[ntp-1]!=nullptr) d.id_tp=otcep->tos->_rc_zam[ntp-1]->id();
    //        if ()
    //    }

    d.v_in=_undefV_;
    d.ms_out_tp=0;
    d.id_tp=0;

    if (otcep->tos->out_tp_rc!=nullptr){
        d.v_out_tp=otcep->tos->out_tp_v;
        d.ms_out_tp=otcep->tos->out_tp_t.msecsTo(T)-ms;
        d.id_tp=otcep->tos->out_tp_rc->id();
    }



    d.x1=x1;
    d.x2=x2;
    d.ms=ms;
    d.v_in=v_in;
    d.v_out=v_out;

    //    if (d.id_tp==0) return;
    if (d.ms<=0) return;
    if (d.ms_out_tp<0) return;
    if (x1>=x2) return;
    if ((d.v_in<0.1) || (d.v_in>30)) return;
    if ((d.v_out<0.1) || (d.v_out>30)) return;
    if (d.v_out-d.v_in>5) return;
    if (d.v_in-d.v_out>10) return;


    if (d.ves==0) return;
    if (d.vag==0) return;
    if (d.marf==0) return;
    if (d.v_in==_undefV_) return;
    //    if (d.dV12==_undefV_) return;
    //    if (d.dV23==_undefV_) return;
    if (otcep->STATE_ERROR()) return;
    // убедимся что не лежим на тп
    foreach (auto rc1, otcep->vBusyRc) {
        if (qobject_cast<m_RC_Gor_Zam *>(rc1)!=nullptr) return;
    }

    add_stat_data(d,_statistic);
}

void DynamicStatistic::add_stat_data(t_rc_stat_data d, int dst)
{

    //if (!m.contains(d.id)) return;
    // dvp
    if (d.v_out==_undefV_) return;
    if (d.v_in==_undefV_) return;
    if (d.v_in<=0) return;
    if ((d.x2-d.x1)<=0) return;
    d.dvp=(d.v_out-d.v_in);
    //    d.dvp=(d.v_out-d.v_in)/d.v_in;
    //    d.e=(d.v_in*d.v_in-d.v_out*d.v_out)/20.;
    //    d.dvp=(d.v_out*d.v_out-d.v_in*d.v_in);
    //    d.dvp=d.dvp/(d.x2-d.x1);
    if (dst==_default){
        rc_stat &stat=mRc2StatDef[d.id];
        if (d.v_out_tp<=0) return;
        for (t_rc_stat_data &dd : stat.l) {
            if ((dd.v_out_tp==d.v_out_tp) && (dd.ves==d.ves) && (dd.vag==d.vag)&& (dd.marf==d.marf)&&
                    (dd.x1==d.x1)&& (dd.x2==d.x2)){
                dd=d;
                return;
            }
        }
        stat.l.push_back(d);
    } else {
        rc_stat &stat=mRc2Stat[d.id];
        for (t_rc_stat_data &dd : stat.l) {
            if (dd.T==d.T) return;
        }
        QList<t_rc_stat_data> ldv=stat_data(d,_statistic, _dv_category);
        if (ldv.size()<_max_rec_in_dv_category){
            stat.l.push_back(d);
        } else {
            std::sort(ldv.begin(),ldv.end(),stat_dataTLessThan);
            stat.l.removeOne(ldv.first());
            stat.l.push_back(d);
        }
        write_stat("./dyn_stat.bin");
    }


}

t_rc_stat_data *DynamicStatistic::default_stat_data(t_rc_stat_data p)
{
    if (!mRc2StatDef.contains(p.id)) return nullptr;
    rc_stat &stat=mRc2StatDef[p.id];
    foreach (const t_rc_stat_data &d, stat.l) {
        if ((p.marf==d.marf)&&
                (p.category_vag()==d.category_vag()) &&
                (p.category_ves()==d.category_ves()) &&
                (p.x1==d.x1) &&
                (p.x2==d.x2) &&
                (p.v_out_tp==d.v_out_tp) ) return (t_rc_stat_data *)&d;
    }
    return nullptr;
}

QList<t_rc_stat_data> DynamicStatistic::stat_data(t_rc_stat_data p,  int dst,qreal dv_out_tp_max)
{
    QList<t_rc_stat_data> l;
    if ((dst==_default)&&(!mRc2StatDef.contains(p.id))) return l;
    if ((dst==_statistic)&&(!mRc2Stat.contains(p.id))) return l;
    rc_stat &stat = (dst==_default)? mRc2StatDef[p.id] : mRc2Stat[p.id];

    foreach (const t_rc_stat_data &d, stat.l) {
        if ((d.marf==p.marf)&&
                (d.category_vag()==p.category_vag()) &&
                (d.category_ves()==p.category_ves())){
            if ((p.x1<d.x1) && (p.x2<d.x1)) continue;
            if ((p.x1>d.x2) && (p.x2>d.x2)) continue;
            if (qFabs(d.v_out_tp-p.v_out)>dv_out_tp_max) continue;
            t_rc_stat_data dd=d;
            dd.x1=p.x1;
            dd.x2=p.x2;
            if (p.x1<d.x1) dd.x1=d.x1;
            if (p.x2>d.x2) dd.x2=d.x2;
            // приводим dvp
            if ((d.x2-d.x1)<=0) continue;
            if ((dd.x2-dd.x1)<=0) continue;
            dd.dvp=d.dvp*(dd.x2-dd.x1)/(d.x2-d.x1);
            l.push_back(dd);
        }


    }
    return l;
}

QList<t_rc_stat_data> DynamicStatistic::stat_data_ms_out_tp(t_rc_stat_data p,  int dst,qreal dv_out_tp_max)
{
    QList<t_rc_stat_data> l;
    if ((dst==_default)&&(!mRc2StatDef.contains(p.id))) return l;
    if ((dst==_statistic)&&(!mRc2Stat.contains(p.id))) return l;
    rc_stat &stat = (dst==_default)? mRc2StatDef[p.id] : mRc2Stat[p.id];

    foreach (const t_rc_stat_data &d, stat.l) {
        if ((d.marf==p.marf)&&
                (d.category_vag()==p.category_vag()) &&
                (d.category_ves()==p.category_ves())){
            if (p.ms_out_tp<d.ms_out_tp) continue;
            if (p.ms_out_tp>d.ms_out_tp+d.ms) continue;
            if (qFabs(d.v_out_tp-p.v_out)>dv_out_tp_max) continue;
            t_rc_stat_data dd=d;
            // приводим x v
            qreal dms=1.*(p.ms_out_tp-d.ms_out_tp)/(1.*d.ms);
            dd.x1=d.x1+dms*(d.x2-d.x1);
            dd.x2=dd.x1;
            dd.v_in=d.v_in+dms*(d.v_out-d.v_in);

            l.push_back(dd);
        }

    }
    return l;
}
bool dv_out_tp_less_then(const t_rc_stat_data &d1,const t_rc_stat_data &d2)
{
    if (d1._dv_out_tp<d2._dv_out_tp) return true;
    return false;
}
qreal DynamicStatistic::aver_dvp(t_rc_stat_data p,QList<t_rc_stat_data> &l)
{
    // находим среднеприблеженное к v_out_tp  в диапазоне x1..x2
    if (l.size()==1) return l.first().dvp;
    for (t_rc_stat_data &d :l){
        d._dv_out_tp=qFabs(p.v_out_tp-d.v_out_tp);
    }

    // сортируем по в вых
    std::sort(l.begin(),l.end(),dv_out_tp_less_then);

    // убираем лишние
    while (l.size()>10) l.removeLast();

    qreal sum_dv_out_tp=0;
    for (t_rc_stat_data &d :l){
        sum_dv_out_tp+=qFabs(p.v_out_tp-d.v_out_tp);
    }
    // перксчитываем dvp относительно v_out_tp
    //  а вот не думаю у нас же процентное отношение
    //  мы не знаем dvp=f(dv)
    //    for (t_rc_stat_data &d :l){
    //        d.dvp=d.dvp * p.v_out_tp/d.v_out_tp;
    //    }


    // рассставляем вес для каждой в вых
    qreal sum_k_out_tp=0;
    qreal sum_dvp=0;
    for (t_rc_stat_data &d :l){
        qreal k=1;
        if (sum_dv_out_tp!=0) k=(1.-d._dv_out_tp/sum_dv_out_tp);
        // можно еще расставить по v_in
        sum_k_out_tp+=k;
        sum_dvp+=d.dvp*k;
    }
    qreal dvp=0;
    if (sum_k_out_tp!=0) dvp=sum_dvp/sum_k_out_tp;
    return dvp;
}


void  aver_xv_ms(t_rc_stat_data p,QList<t_rc_stat_data> &l,qreal &x,qreal &v)
{

    x=0;
    v=_undefV_;
    // находим среднеприблеженное к v_out_tp  в диапазоне ms..
    if (l.size()==1) {
        v=l.first().v_in;
        x=l.first().x1;
        return ;
    }
    for (t_rc_stat_data &d :l){
        d._dv_out_tp=qFabs(p.v_out_tp-d.v_out_tp);
    }

    // сортируем по в вых
    std::sort(l.begin(),l.end(),dv_out_tp_less_then);

    // убираем лишние
    while (l.size()>10) l.removeLast();

    qreal sum_dv_out_tp=0;
    for (t_rc_stat_data &d :l){
        sum_dv_out_tp+=qFabs(p.v_out_tp-d.v_out_tp);
    }
    // рассставляем вес для каждой в вых
    qreal sum_k_out_tp=0;
    qreal sum_x=0;
    qreal sum_v=0;
    //    qreal sum_v_out_tp=0;
    for (t_rc_stat_data &d :l){
        qreal k=1;
        if (sum_dv_out_tp!=0) k=(1.-d._dv_out_tp/sum_dv_out_tp);
        sum_k_out_tp+=k;
        //        sum_v_out_tp+=k*d.v_out_tp;
        sum_x+=k*d.x1;
        sum_v+=k*d.v_in;
    }

    if (sum_k_out_tp!=0) {
        v=sum_v/sum_k_out_tp;
        x=sum_x/sum_k_out_tp;
    }
}
qreal DynamicStatistic::prognoz_v_out(t_rc_stat_data p, qreal v_in,  int dst)
{
    qreal dvp=prognoz_dvp(p, dst);
    qreal v_out=0;
    //    if ((v_in*v_in+dvp)>0) v_out=qSqrt(v_in*v_in+dvp);
    //    v_out=v_in+v_in*dvp;
    v_out=v_in+dvp;
    return v_out;
}
qreal DynamicStatistic::prognoz_dvp(t_rc_stat_data p,  int dst)
{
    QList<t_rc_stat_data> l= stat_data( p, dst,20);
    qreal dvp=aver_dvp(p,l);

    return dvp;
}

qreal DynamicStatistic::prognoz_v_out(m_Otcep *otcep,m_RC *rc,qreal x1,qreal x2, qreal v_in)
{
    t_rc_stat_data p;
    p.x1=x1;
    p.x2=x2;
    p.marf=otcep->STATE_MAR();
    p.id=rc->id();
    p.vag=otcep->STATE_VAGON_CNT();
    p.ves=otcep->STATE_VES();
    QList<t_rc_stat_data> l= stat_data( p, _statistic,4);
    qreal dvp=0;
    if (l.size()>=3) {
        dvp=aver_dvp(p,l);
    } else {
        l.clear();
        l= stat_data( p, _default,20);
        dvp=aver_dvp(p,l);
    }
    return v_in+dvp;

}

void DynamicStatistic::prognoz_ms_xv(m_Otcep *otcep, m_RC *rc, qint64 ms_out_tp, qreal &x, qreal &v)
{
    t_rc_stat_data p;
    p.ms_out_tp=ms_out_tp;
    p.marf=otcep->STATE_MAR();
    p.id=rc->id();
    p.vag=otcep->STATE_VAGON_CNT();
    p.ves=otcep->STATE_VES();
    QList<t_rc_stat_data> l= stat_data_ms_out_tp( p, _statistic,4);
    if (l.size()>=3) {
        aver_xv_ms(p,l,x,v);
    } else {
        l.clear();
        l= stat_data_ms_out_tp( p, _default,20);
        aver_xv_ms(p,l,x,v);
    }
}



void DynamicStatistic::work(const QDateTime &T)
{


    foreach (auto rc_park, l_rc_park) {
        if (rc_park->rcs->l_otceps.isEmpty()) continue;
        m_Otcep *otcep=rc_park->rcs->l_otceps.last();
        if (otcep->RCF!=rc_park) continue;


        //    foreach (m_Otcep *otcep, TOS->lo) {
        //        if ((otcep->RCF)&&(qobject_cast<m_RC_Gor_Park*>(otcep->RCF)!=nullptr)){
        // кзп
        if ((otcep->STATE_KZP_OS()==m_Otcep::kzpMoving) &&(otcep->STATE_KZP_D()<=_ds_parck_div)) otcep->tos->_stat_kzp_t=otcep->tos->out_tp_t;
        if ((otcep->STATE_KZP_OS()==m_Otcep::kzpMoving) &&
                (otcep->STATE_KZP_D()>=_ds_parck_div) &&
                (otcep->STATE_KZP_D()-otcep->tos->_stat_kzp_d>=_ds_parck_div)){

            qint64 ms=otcep->tos->_stat_kzp_t.msecsTo(T);
            // приводим к разбивке
            if (ms<=0) continue;
            //            qreal ms1=ms;
            //            if ((otcep->STATE_KZP_D()-otcep->tos->_stat_kzp_d)>_ds_parck_div){
            //                qreal v=tos_SpeedCalc::calcV(ms,otcep->STATE_KZP_D()-otcep->tos->_stat_kzp_d);
            //                if (v<=0) continue;
            //                ms1=3600.*1.*_ds_parck_div/v;
            //            }
            //            int k=(otcep->STATE_KZP_D()-otcep->tos->_stat_kzp_d)/_ds_parck_div;
            //            qreal v_out=tos_SpeedCalc::calcV(ms1,_ds_parck_div);
            //            qreal v_in=otcep->tos->STATE_V_RCF_IN();
            //            qreal dv=(v_out-v_in)/k;
            //            for (int i=0;i<k;i++){
            //                qreal x1=otcep->tos->_stat_kzp_d+(i+0)*_ds_parck_div;
            //                qreal x2=otcep->tos->_stat_kzp_d+(i+1)*_ds_parck_div;
            //                qreal v_in1=v_in+   dv*1.*(k-1);
            //                qreal v_out1=v_in+ dv*1.*k;
            //                add_stat(T,otcep,otcep->RCF,x1,x2,ms1,v_in1,v_out1);

            //            }

            // упрощаю
            int d=otcep->STATE_KZP_D();
            if (otcep->STATE_KZP_D()% _ds_parck_div>0)
                d=otcep->STATE_KZP_D()/ _ds_parck_div*_ds_parck_div;
            qreal v_out=tos_SpeedCalc::calcV(ms,_ds_parck_div);
            qreal v_in=otcep->tos->STATE_V_RCF_IN();
            qreal x1=otcep->tos->_stat_kzp_d;
            qreal x2=d;
            otcep->tos->setSTATE_V_RCF_IN(v_out);
            otcep->tos->_stat_kzp_d=d;
            otcep->tos->_stat_kzp_t=T;
            add_stat(T,otcep,otcep->RCF,x1,x2,ms,v_in,v_out);
        }
        continue;

    }

}



void DynamicStatistic::resetStates()
{
    //        BaseWorker::resetStates();
    //    foreach (m_Otcep *otcep, TOS->lo) {

    //    }
}

