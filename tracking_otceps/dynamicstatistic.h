#ifndef DYNAMICSTATISTIC_H
#define DYNAMICSTATISTIC_H

#include "trackingotcepsystem.h"

enum {_ds_parck_div=50};
enum {
    _default=0,
    _statistic=1
};

#pragma pack(push, 1)
struct t_rc_stat_data
{
    QDateTime T;
    quint64 id=0; // рц
    qreal absx=0;

    int ves=0;      // разбивка по категориям
    int vag=0;
    int marf=0;
    qreal dV12=0;   // на будующее попытка определения бегуна
    qreal dV23=0;

    // что реально собрать

    qreal v_out_tp=0;  // скорость выходы с ТП  от нее и пляшем
    quint64 id_tp=0;
    qint64 ms_out_tp=0; //


    qreal v_in=0;  // скрость входа на рц
    qreal x1=0;
    qreal x2=0;
    qint64 ms=0; // время движения от x1 - x2
    qreal v_out=0;  // скрость входа на рц

    qreal dvp=0; // потеря скорости по отношению входа (v_out-v_in)/v_in
                    // для _default выставляется, для _statistic высчитывается

    qreal dvpl()const {return 50./(x2-x1) * dvp;}
    void set_dvpl(qreal p){dvp=p/50.*(x2-x1);}

//    qreal v_out_rc() const     {
//        if (ms==0) return _undefV_;
//        return 3600.*(x2-x1)/ms;
//    }


    int category_ves() const {
        if (ves<=0) return 0;
        if (ves<75) return 1;
        if (ves<125) return 2;
        if (ves<175) return 3;
        if (ves<400) return 4;
        return 0;
    }
    void set_category_ves(int p){
        if (p==1) ves=35;
        if (p==2) ves=100;
        if (p==3) ves=150;
        if (p==4) ves=200;
    }
    int category_vag() const {
        if (vag<=0) return 0;
        if (vag==1) return 1;
        if (vag==2) return 2;
        if (vag<=4) return 3;
        return 4;
    }
    void set_category_vag(int p){
        if (p==1) vag=1;
        if (p==2) vag=2;
        if (p==3) vag=4;
        if (p==4) vag=8;
    }
    bool	operator == (const t_rc_stat_data &other) const {
        if (memcmp(this,&other,sizeof(t_rc_stat_data))==0) return true;
        return false;
    }
    qreal _dv_out_tp=0;
    m_RC *_rc=nullptr;
};

#pragma pack(pop)



struct rc_stat{
    QList<t_rc_stat_data> l;
};



class DynamicStatistic : public BaseWorker
{
    Q_OBJECT
public:
    explicit DynamicStatistic(QObject *parent,TrackingOtcepSystem *TOS);
    virtual ~DynamicStatistic(){}
    virtual void work(const QDateTime &T);
    virtual void resetStates();
    bool load_stat(QString fn);
    bool write_stat(QString fn);
    qreal defaultVprognoz(t_rc_stat_data categ_d,m_RC *rc);
    void makeDefault();
    QList<t_rc_stat_data> stat_data(t_rc_stat_data, int dst,qreal dv_out_tp_max);
    QList<t_rc_stat_data> stat_data_ms_out_tp(t_rc_stat_data p,  int dst,qreal dv_out_tp_max);

     QMap<quint64,rc_stat> mRc2Stat;
     QMap<quint64,rc_stat> mRc2StatDef;
     TrackingOtcepSystem *TOS;
     void add_stat_data(t_rc_stat_data d,int dst);
     t_rc_stat_data *default_stat_data(t_rc_stat_data d);
     qreal prognoz_v_out(t_rc_stat_data p, qreal v_in,  int dst);
     qreal prognoz_dvp(t_rc_stat_data p,  int dst);
     qreal aver_dvp(t_rc_stat_data p,QList<t_rc_stat_data> &l);

     qreal prognoz_v_out(m_Otcep *otcep,m_RC *rc,qreal x1,qreal x2, qreal v_in);
     void prognoz_ms_xv(m_Otcep *otcep,m_RC *rc,qint64 ms_out_tp, qreal &x,qreal &v);

     QList<t_rc_stat_data> rc_marshrut(int put_nadvig,int m);

signals:
public slots:
    void slot_otcep_rcsf_change(m_Otcep *otcep,int sf,m_RC*rcFrom,m_RC*rcTo,QDateTime T,QDateTime Trc);
protected:


    QMap<m_RC*,int> mRc2NTP;
    QList<m_RC_Gor_Park*> l_rc_park;
    void add_stat(const QDateTime &T, m_Otcep *otcep, m_RC *rc, int x1, int x2, qint64 ms, qreal v_in, qreal v_out);

};


#endif // DYNAMICSTATISTIC_H
