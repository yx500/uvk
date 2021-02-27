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
    foreach (auto strel_Y, l_strel_Y) {
        NgbStrel *ngb=new NgbStrel;
        ngb->strel=strel_Y;
        ngb->trc=mRc2TRC[strel_Y];
        for (int m=0;m<2;m++){
            ngb->l_ngb_trc[m].clear();
            foreach (auto rc, strel_Y->l_ngb_rc[m]) {
                ngb->l_ngb_trc[m].push_back(mRc2TRC[rc]);
            }
        }
        l_NgbStrel.push_back(ngb);
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

    //    foreach (auto trc, l_trc) {
    //        >>tos_System::acceptOutputSignals();
    //    }

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
        trc->state2buffer();
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

    // снимаем  негабариты
    resetNGB();
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
            // 1  не может быть одна -  достаточно по 1 рц
            bool nesnimat=false;
            for (int d=0;d<2;d++){
                auto rc1=trc->next_rc[d];
                if ((rc1==nullptr)||(!rc1->l_os.isEmpty())) {nesnimat=true;break;}
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

void tos_System_DSO::setDSOBUSY(const QDateTime &)
{
    foreach (auto trc, l_trc) {
        if (trc->l_os.isEmpty())
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::free); else
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::busy);
    }
}

void tos_System_DSO::resetNGB()
{
    foreach (auto strel, l_strel_Y) {
        bool  ex_busy=false;
        foreach (auto rc, strel->l_ngb_rc[0]) {
            if (rc->STATE_BUSY()==1) ex_busy=true;
        }
        if (!ex_busy){
            strel->setSTATE_UVK_NGBDYN_PL(false);
            strel->setSTATE_UVK_NGBSTAT_PL(false);
        }
        ex_busy=false;
        foreach (auto rc, strel->l_ngb_rc[1]) {
            if (rc->STATE_BUSY()==1) ex_busy=true;
        }
        if (!ex_busy){
            strel->setSTATE_UVK_NGBDYN_MN(false);
            strel->setSTATE_UVK_NGBSTAT_MN(false);
        }
    }
}

void tos_System_DSO::setNGBDYN(const QDateTime &)
{
    // выставляем для стрелок где скорость меньше гран
    foreach (auto ngb, l_NgbStrel) {
        if (ngb->trc->l_os.isEmpty()) continue;
        if ((ngb->trc->v_dso!=_undefV_) && (ngb->trc->v_dso>=0)){
            auto strel=qobject_cast<m_Strel_Gor_Y*>(ngb->trc->rc);
            if (strel->STATE_POL()==MVP_Enums::pol_plus){
                if ((!ngb->l_ngb_trc[0].isEmpty())&&(ngb->l_ngb_trc[0].first()->rc->STATE_BUSY()==MVP_Enums::busy)){
                    if (ngb->trc->v_dso<strel->NEGAB_VGRAN_P()) strel->setSTATE_UVK_NGBDYN_PL(true); else  strel->setSTATE_UVK_NGBDYN_PL(false);
                }
            }
            if (strel->STATE_POL()==MVP_Enums::pol_minus){
                if ((!ngb->l_ngb_trc[1].isEmpty())&&(ngb->l_ngb_trc[1].first()->rc->STATE_BUSY()==MVP_Enums::busy)){
                    if (ngb->trc->v_dso<strel->NEGAB_VGRAN_M()) strel->setSTATE_UVK_NGBDYN_MN(true);else  strel->setSTATE_UVK_NGBDYN_MN(false);
                }
            }
        }
    }
}

void tos_System_DSO::setNGBSTAT(const QDateTime &)
{
    foreach (auto ngb, l_NgbStrel) {
        for (int m=0;m<2;m++){
            foreach (auto trc, ngb->l_ngb_trc[m]) {
                if ((trc->rc->STATE_BUSY_DSO_STOP())|| (trc->rc->STATE_BUSY_DSO_OSTOP())){
                    if (m==0) ngb->strel->setSTATE_UVK_NGBSTAT_PL(true);
                    if (m==1) ngb->strel->setSTATE_UVK_NGBSTAT_MN(true);
                }
            }
        }
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

        if (act_zkr!=nullptr){
            if ((otcep->STATE_MAR()>0)&&((otcep->STATE_GAC_ACTIVE())||(otcep->STATE_LOCATION()==m_Otcep::locationOnPrib))) {
                auto m=modelGorka->getMarshrut(act_zkr->PUT_NADVIG(),otcep->STATE_MAR());
                // идем по маршруту
                bool begin_found=false;
                if (otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) begin_found=true;
                for (const RcInMarsrut&mr :m->l_rc){
                    // идем по маршруту с головы отцепа
                    if (!begin_found){
                        if (mr.rc==otcep->RCS) begin_found=true;
                        continue;
                    }
                    if (warn1==0){
                        // стрелки в среднем положении
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
                        // что то стоит на пути
                        if ((mr.rc->MINWAY()!=mr.rc->MAXWAY())&&
                                ((mr.rc->STATE_BUSY_DSO_ERR())||(mr.rc->STATE_BUSY_DSO_STOP()))
                                ){
                            warn2=1;
                        }
                        // негабаритность
                        m_Strel_Gor_Y* str=qobject_cast<m_Strel_Gor_Y*>(mr.rc) ;
                        if (str!=nullptr){
                            if ((str->STATE_UVK_NGBDYN_PL()==1) && (mr.pol==MVP_Enums::pol_minus)) warn1=1;
                            if ((str->STATE_UVK_NGBDYN_MN()==1) && (mr.pol==MVP_Enums::pol_plus))  warn1=1;
                            if ((str->STATE_UVK_NGBSTAT_PL()==1) && (mr.pol==MVP_Enums::pol_minus)) warn1=1;
                            if ((str->STATE_UVK_NGBSTAT_MN()==1) && (mr.pol==MVP_Enums::pol_plus))  warn1=1;
                        }

                    }
                }
            }
        }
        otcep->setSTATE_GAC_W_STRA(warn1);
        otcep->setSTATE_GAC_W_STRB(warn2);
    }
}
