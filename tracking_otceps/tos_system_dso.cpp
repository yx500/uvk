#include "tos_system_dso.h"

#include <qdebug.h>
#include <QtMath>
#include <assert.h>
#include "baseobjecttools.h"
#include "mvp_system.h"

#include "tos_speedcalc.h"
#include "m_otceps.h"
#include "m_zam.h"

extern int testMode;

tos_System_DSO::tos_System_DSO(QObject *parent) :
    tos_System(parent)
{
    iGetNewOtcep=nullptr;
    makeWorkers(modelGorka);
}

void tos_System_DSO::validation(ListObjStr *l) const
{
    tos_System::validation(l);

    foreach (auto w, l_dso) {
        w->validation(l);
    }
    foreach (auto w, l_tdso) {
        w->validation(l);
    }
    foreach (auto w, l_trdso) {
        w->validation(l);
    }
    foreach (auto w, l_trdso) {
        w->validation(l);
    }
    foreach (auto w, l_tzkr) {
        w->validation(l);
    }
}


void tos_System_DSO::makeWorkers(ModelGroupGorka *modelGorka)
{
    tos_System::makeWorkers(modelGorka);
    // собираем все ДСО

    l_dso=modelGorka->findChildren<m_DSO_RD_21*>();

    foreach (auto dso, l_dso) {
        tos_DSO *tdso=new tos_DSO(this,dso);
        l_tdso.push_back(tdso);
        mDSO2TDSO[dso]=tdso;
        if ((dso->rc_next[0]!=nullptr)&&(dso->rc_next[1]!=nullptr)){
            // проверяем на повтор
            bool ex=false;
            foreach (auto trdso1, l_trdso) {
                if ((trdso1->tdso->dso->rc_next[0]==dso->rc_next[0])&&
                        (trdso1->tdso->dso->rc_next[1]==dso->rc_next[1])){
                    trdso1->add2DSO(tdso);
                    mDSO2TRDSO[dso]=trdso1;
                    ex=true;
                    break;
                }
            }
            if (!ex) {
                tos_DsoTracking *trdso=new tos_DsoTracking(this,tdso);
                mDSO2TRDSO[dso]=trdso;
                l_trdso.push_back(trdso);
            }

        }

    }

    foreach (auto tdso, l_tdso) {
        if ((tdso->dso->rc!=nullptr)&&(tdso->dso->dso_pair!=nullptr)){
            tos_DsoPair * tdsop=new tos_DsoPair(this,tdso,mDSO2TDSO[tdso->dso->dso_pair]);
            l_tdsopair.push_back(tdsop);
        }
    }

    foreach (auto zkr, l_zkr) {
        tos_Zkr_DSO *tzkr=new tos_Zkr_DSO(this,mRc2TRC[zkr]);
        l_tzkr.push_back(tzkr);
    }
    foreach (auto rc_park, l_park) {
        l_trc_park.push_back(mRc2TRC[rc_park]);
    }





    resetStates();
}

void tos_System_DSO::resetStates()
{
    tos_System::resetStates();
    foreach (auto w, l_dso) {
        w->resetStates();
    }
    foreach (auto w, l_tdso) {
        w->resetStates();
    }
    foreach (auto w, l_trdso) {
        w->resetStates();
    }
    foreach (auto w, l_trdso) {
        w->resetStates();
    }
    foreach (auto w, l_tdsopair) {
        w->resetStates();
    }
    foreach (auto w, l_tzkr) {
        w->resetStates();
    }

    foreach (auto w, l_tdsopair) {
        if (w->strel!=nullptr){
            w->strel->setSTATE_UVK_TLG(false);
        }
    }

    if (testMode==0){
        foreach (auto w, l_trc) {
            w->rc->setSTATE_BUSY_DSO_ERR(true);
        }
    }



}
QList<SignalDescription> tos_System_DSO::acceptOutputSignals()
{

    auto l=tos_System::acceptOutputSignals();

    foreach (auto trc, l_trc) {
        trc->rc->setSIGNAL_BUSY_DSO(trc->rc->SIGNAL_BUSY_DSO().innerUse());         l<<trc->rc->SIGNAL_BUSY_DSO();
        trc->rc->setSIGNAL_BUSY_DSO_ERR(trc->rc->SIGNAL_BUSY_DSO_ERR().innerUse()); l<<trc->rc->SIGNAL_BUSY_DSO_ERR();
        trc->rc->setSIGNAL_INFO_DSO(trc->rc->SIGNAL_INFO_DSO().innerUse());         l<<trc->rc->SIGNAL_INFO_DSO();
        trc->rc->setSIGNAL_BUSY_DSO_STOP(trc->rc->SIGNAL_BUSY_DSO_STOP().innerUse());l<<trc->rc->SIGNAL_BUSY_DSO_STOP();

        trc->rc->SIGNAL_INFO_DSO().getBuffer()->setSizeData(DSO_Data_Max*sizeof(DSO_Data));
    }

    foreach (auto w, l_tdso) {
        l+=w->acceptOutputSignals();
    }
    foreach (auto w, l_trdso) {
        l+=w->acceptOutputSignals();
    }
    foreach (auto w, l_tzkr) {
        l+=w->acceptOutputSignals();
    }

    foreach (auto w, l_tdsopair) {
        if (w->strel!=nullptr){
            w->strel->setSIGNAL_UVK_TLG(w->strel->SIGNAL_UVK_TLG().innerUse());
            l<< w->strel->SIGNAL_UVK_TLG();

            w->strel->setSIGNAL_UVK_NGBDYN_PL(w->strel->SIGNAL_UVK_NGBDYN_PL().innerUse());
            w->strel->setSIGNAL_UVK_NGBDYN_MN(w->strel->SIGNAL_UVK_NGBDYN_MN().innerUse());
            l<< w->strel->SIGNAL_UVK_NGBDYN_PL();
            l<< w->strel->SIGNAL_UVK_NGBDYN_MN();
        }
    }

    return l;
}
void tos_System_DSO::state2buffer()
{
    tos_System::state2buffer();

    foreach (auto trc, l_trc) {
        trc->rc->SIGNAL_BUSY_DSO().setValue_1bit(trc->rc->STATE_BUSY_DSO());
        trc->rc->SIGNAL_BUSY_DSO_ERR().setValue_1bit(trc->rc->STATE_BUSY_DSO_ERR());
        trc->rc->SIGNAL_BUSY_DSO_STOP().setValue_1bit(trc->rc->STATE_BUSY_DSO_STOP());
        trc->rc->SIGNAL_ERR_LS().setValue_1bit(trc->rc->STATE_ERR_LS());
        trc->rc->SIGNAL_ERR_LZ().setValue_1bit(trc->rc->STATE_ERR_LZ());
        DSO_Data d;
        d.V=trc->l_os.size();
        trc->rc->SIGNAL_INFO_DSO().setValue_data(&d,sizeof (d));

    }

    foreach (auto w, l_tdso) {
        w->state2buffer();
    }
    foreach (auto w, l_trdso) {
        w->state2buffer();
    }
    foreach (auto w, l_tzkr) {
        w->state2buffer();
    }

    foreach (auto w, l_tdsopair) {
        if (w->strel!=nullptr){
            w->strel->SIGNAL_UVK_TLG().setValue_1bit(w->strel->STATE_UVK_TLG());
            w->strel->SIGNAL_UVK_NGBDYN_PL().setValue_1bit(w->strel->STATE_UVK_NGBDYN_PL());
            w->strel->SIGNAL_UVK_NGBDYN_MN().setValue_1bit(w->strel->STATE_UVK_NGBDYN_MN());
        }
    }
}


void tos_System_DSO::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;

    // состояние рц
    foreach (auto trc, l_trc) {
        trc->work(T);
    }

    // датчики
    foreach (auto w, l_tdso) {
        w->work(T);
    }

    // телеги
    foreach (auto w, l_tdsopair) {
        w->work(T);
    }

    // зкр
    foreach (auto w, l_tzkr) {
        w->work(T);
    }

    // переходы
    foreach (auto w, l_trdso) {
        w->work(T);
    }

    // кзп

    // сбрасываем сбойную ось
    reset_1_os(T);

    // выставляем занятость по осям
    setDSOBUSY(T);

    // выставляем динамический негабарит
    setNGBDYN(T);

    // выставляем занятость телегами
    foreach (auto w, l_tdsopair) {
        if (w->strel!=nullptr){
            if (w->sost==tos_DsoPair::_sost_wait2t)
                w->strel->setSTATE_UVK_TLG(true);else
                w->strel->setSTATE_UVK_TLG(false);
        }
    }


    // раставляем по рц
    updateOtcepsOnRc(T);

    // выставляем параметра отцепов
    updateOtcepsParams(T);

    // выставляем предупреждения отцепов
    set_otcep_STATE_WARN(T);

}


void tos_System_DSO::updateOtcepsOnRc(const QDateTime &)
{
    // проставялем начало и конец по номеру оси

    foreach (auto o, lo) {
        m_RC *rcs=nullptr;
        m_RC *rcf=nullptr;
        TOtcepDataOs s_os;
        TOtcepDataOs f_os;
        if (o->otcep->STATE_LOCATION()==m_Otcep::locationUnknow) continue;
        if (o->otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) continue;
        TOtcepDataOs lstOs;
        lstOs.v=_undefV_;
        foreach (auto trc, l_trc) {
            for (TOtcepDataOs &os :trc->l_os){
                if (os.num==o->otcep->NUM()){

                    if ((!rcs)||(os.os_otcep<s_os.os_otcep)){
                        rcs=trc->rc;s_os=os;
                    }
                    if ((!rcf)||(os.os_otcep>f_os.os_otcep)){
                        rcf=trc->rc;f_os=os;
                    }
                    if (((!lstOs.t.isValid())||(lstOs.t<os.t))&&(os.v!=_undefV_)){
                        lstOs=os;
                    }
                }
            }
        }
        if ((rcs==nullptr)&&(o->otcep->RCS!=nullptr)){
            resetTracking(o->otcep->NUM());
        } else {
            if ((rcs!=o->otcep->RCS) || (rcf!=o->otcep->RCF)){
                o->otcep->RCS=rcs;
                o->otcep->RCF=rcf;
                o->otcep->setBusyRC();
                if (s_os.t>f_os.t) o->otcep->setSTATE_DIRECTION(s_os.d); else
                    o->otcep->setSTATE_DIRECTION(f_os.d);
            }
        }
        o->otcep->setSTATE_V_DISO(lstOs.v);
    }

    foreach (auto trc, l_trc) {
        trc->l_otceps.clear();
        for (TOtcepDataOs &os :trc->l_os){
            if (!trc->l_otceps.contains(os.num)) {
                trc->l_otceps.push_back(os.num);
            }
        }
    }
}

void tos_System_DSO::resetTracking(int num)
{
    tos_System::resetTracking(num);

    foreach (auto trc, l_trc) {
        if (trc->l_otceps.contains(num)) trc->l_otceps.removeAll(num);
        for (TOtcepDataOs &os :trc->l_os){
            if (os.num==num) os.num=0;
        }
    }

    reset_1_os(QDateTime::currentDateTime());
}

void tos_System_DSO::resetTracking()
{
    tos_System::resetTracking();
    //
    foreach (auto trc, l_trc) {
        trc->l_otceps.clear();
        for (TOtcepDataOs &os :trc->l_os){
            os.num=0;
        }
        trc->rc->setSTATE_ERR_LZ(false);
        trc->rc->setSTATE_ERR_LS(false);
    }

    foreach (auto trc, l_trc_park) {
        trc->l_os.clear();
    }

    //    foreach (auto w, l_dso) {
    //        w->resetStates();
    //    }
    foreach (auto w, l_tdso) {
        w->resetTracking();
    }
    foreach (auto w, l_trdso) {
        w->resetStates();
    }

    foreach (auto w, l_tzkr) {
        w->resetStates();
    }
}

bool tos_System_DSO::resetDSOBUSY(QString idtsr, QString &acceptStr)
{
    int n=0;
    foreach (auto trc, l_trc) {
        if ((idtsr=="*") || (trc->rc->idstr()==idtsr)){
            trc->l_os.clear();
            trc->rc->setSTATE_BUSY_DSO_ERR(false);
            n++;
        }
    }
    acceptStr=QString("Снята занятость на %1 РЦ .").arg(n);
    updateOtcepsOnRc(QDateTime());
    return true;
}

void tos_System_DSO::reset_1_os(const QDateTime &T)
{
    foreach (auto trc, l_trc) {
        if (trc->l_os.size()==1){
            // прихолится рассматривать по 2 от рц так как могут быиь длиннобазы
            bool nesnimat=false;
            for (int d=0;d<2;d++){
                auto rc1=trc->next_rc[d];
                if ((rc1==nullptr)||(!rc1->l_os.isEmpty())) {nesnimat=true;break;}
                auto rc2=rc1->next_rc[d];
                if ((rc2==nullptr)||(!rc2->l_os.isEmpty())) {nesnimat=true;break;}
            }
            if (nesnimat) continue;
            auto os=trc->l_os.first();
            if (os.t.msecsTo(T)>1000){
                trc->l_os.clear();
                trc->rc->setSTATE_ERR_LZ(true);
            }
        }
    }
}

void tos_System_DSO::setDSOBUSY(const QDateTime &T)
{
    foreach (auto trc, l_trc) {
        if (trc->l_os.isEmpty())
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::free); else
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::busy);
    }
    // выставляем признак остановки
    foreach (auto trc, l_trc) {
        if (trc->l_os.isEmpty()){
            trc->rc->setSTATE_BUSY_DSO_STOP(false);
            continue;
        }
        // находим ос с макс времнем
        auto t0=trc->l_os.first().t;
        for (const TOtcepDataOs & os : trc->l_os){
            if (os.t>t0) t0=os.t;
        }
        qreal len_rc=40;
        if (trc->rc->LEN()!=0) len_rc=trc->rc->LEN();
        auto ms=t0.msecsTo(T);
        qint64 ms0=len_rc*3600; // 1 км/ч
        if (ms>ms0) trc->rc->setSTATE_BUSY_DSO_STOP(true);
    }

}

void tos_System_DSO::setNGBDYN(const QDateTime &T)
{
    foreach (auto trcd, l_trdso) {
        if (trcd->tdso==nullptr) continue;
        if (trcd->tdso->dso->NGBDYN_OFFSET()==0) continue;
        if ((trcd->rc_next[0]==nullptr) || (trcd->rc_next[1]==nullptr)) continue;
        auto strel=qobject_cast<m_Strel_Gor_Y*>(trcd->rc_next[1]);
        if (strel==nullptr) continue;
        int m=3;
        if (strel->getNextRC(0,0)==trcd->rc_next[0]->rc) m=0;
        if (strel->getNextRC(0,1)==trcd->rc_next[0]->rc) m=1;
        if (m==3) continue;
        bool ngb=false;
        if (!trcd->rc_next[0]->l_os.isEmpty()){
            auto os=trcd->rc_next[0]->l_os.back();
            if ( (os.d!=_forw) &&
                 (os.v!=_undefV_) &&
                 (os.v>0) ){
                auto ms=os.t.msecsTo(T);
                if (ms>0){
                    qint64 ms0=trcd->tdso->dso->NGBDYN_OFFSET()/os.v*3600.;
                    if (ms<ms0) ngb=true;
                }
            }

        }
        if (m==0) strel->setSTATE_UVK_NGBDYN_PL(ngb);
        if (m==1) strel->setSTATE_UVK_NGBDYN_MN(ngb);
    }
}

TOtcepDataOs tos_System_DSO::moveOs(tos_Rc *rc0, tos_Rc *rc1, int d,qreal os_v,const QDateTime &T)
{
    //  0 <-- 1  <<D0
    TOtcepDataOs moved_os;
    if (d==_forw){
        if ((rc1==nullptr) || (rc1->l_os.isEmpty())){
            if ((rc1!=nullptr) && (rc1->l_os.isEmpty())&&(rc1->next_rc[1]!=nullptr))
                rc1->rc->setSTATE_ERR_LS(true);
            // рождение ноаовой оси
            TOtcepDataOs new_os;

            moved_os=new_os;
        } else {
            moved_os=rc1->l_os.first();
            rc1->l_os.pop_front();

        }
        moved_os.t=T;
        moved_os.d=d;
        moved_os.v=os_v;
        if (rc0!=nullptr) {
            rc0->l_os.push_back(moved_os);
            if (moved_os.num!=0){
                if (!rc0->l_otceps.contains(moved_os.num))
                    rc0->l_otceps.push_back(moved_os.num);
            }
        }
    }
    //  0 --> 1  <<D0
    if (d==_back){
        if ((rc0==nullptr) || (rc0->l_os.isEmpty())){
            if ((rc0!=nullptr) && (rc0->l_os.isEmpty())&&(rc0->next_rc[0]!=nullptr))
                rc0->rc->setSTATE_ERR_LS(true);
            // рождение ноаовой оси
            TOtcepDataOs new_os;
            moved_os=new_os;
        } else {
            moved_os=rc0->l_os.last();
            rc0->l_os.pop_back();

        }
        moved_os.t=T;
        moved_os.d=d;
        moved_os.v=os_v;
        if (rc1!=nullptr) {
            rc1->l_os.push_front(moved_os);
            if (moved_os.num!=0){
                if (!rc0->l_otceps.contains(moved_os.num))
                    rc0->l_otceps.push_front(moved_os.num);
            }
        }
    }
    return moved_os;
}


tos_Rc *tos_System_DSO::getRc(TOtcepDataOs os)
{
    foreach (auto trc, l_trc) {
        foreach (auto os1, trc->l_os) {
            if (os1==os) return  trc;
        }
    }return nullptr;
}

void tos_System_DSO::set_otcep_STATE_WARN(const QDateTime &)
{
    auto act_zkr=modelGorka->active_zkr();
    foreach (auto od, lo) {
        auto otcep=od->otcep;
        int warn1=0;
        int warn2=0;
        if (!otcep->STATE_ENABLED()) continue;
        // стрелки в среднем положении
        if (act_zkr!=nullptr){
            if ((otcep->STATE_GAC_ACTIVE())||(otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) {
                if (otcep->STATE_MAR()>0){
                    auto m=modelGorka->getMarshrut(act_zkr->PUT_NADVIG(),otcep->STATE_MAR());
                    // идем по маршруту
                    bool b=false;
                    if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib) b=true;
                    for (const RcInMarsrut&mr :m->l_rc){
                        if (!b){
                            if (mr.rc==otcep->RCS) {
                                b=true;
                                continue;
                            }
                        } else {
                            if (warn1==0){
                                m_Strel_Gor_Y* str=qobject_cast<m_Strel_Gor_Y*>(mr.rc) ;
                                if (str!=nullptr){
                                    if ((str->STATE_A()==0) && (str->STATE_POL()!=mr.pol)){
                                        warn1=1;
                                    }
                                }
                                auto str1=qobject_cast<m_Strel_Gor*>(mr.rc) ;
                                if (str1!=nullptr){
                                    if (str1->STATE_POL()!=mr.pol){
                                        warn1=1;
                                    }
                                }
                            }
                            if (warn2==0){
                                if ((mr.rc->MINWAY()!=mr.rc->MAXWAY())&&
                                        ((mr.rc->STATE_BUSY_DSO_ERR())||(mr.rc->STATE_BUSY_DSO_STOP()))
                                         ){
                                    warn2=1;
                                }
                            }
                        }
                    }
                }

            }
        }
        otcep->setSTATE_GAC_W_STRA(warn1);
        otcep->setSTATE_GAC_W_STRB(warn2);
    }
}
