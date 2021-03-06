#include "tos_system_dso.h"

#include <qdebug.h>
#include <QtMath>
#include <assert.h>
#include "baseobjecttools.h"
#include "mvp_system.h"

//#include "tos_speedcalc.h"
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
            // проверяем на дубль
            bool ex=false;
            foreach (auto trdso1, l_trdso) {
                if (trdso1->tdso->dso->dso_dubl==dso){
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

    //    foreach (auto w, l_tdsopair) {
    //        if (w->strel!=nullptr){
    //            w->strel->setSIGNAL_UVK_TLG(w->strel->SIGNAL_UVK_TLG().innerUse());l<< w->strel->SIGNAL_UVK_TLG();


    //            w->strel->setSIGNAL_UVK_NGBDYN_PL(w->strel->SIGNAL_UVK_NGBDYN_PL().innerUse());l<< w->strel->SIGNAL_UVK_NGBDYN_PL();
    //            w->strel->setSIGNAL_UVK_NGBDYN_MN(w->strel->SIGNAL_UVK_NGBDYN_MN().innerUse());l<< w->strel->SIGNAL_UVK_NGBDYN_MN();
    //            w->strel->setSIGNAL_UVK_NGBSTAT_PL(w->strel->SIGNAL_UVK_NGBSTAT_PL().innerUse());l<< w->strel->SIGNAL_UVK_NGBSTAT_PL();
    //            w->strel->setSIGNAL_UVK_NGBSTAT_MN(w->strel->SIGNAL_UVK_NGBSTAT_MN().innerUse());l<< w->strel->SIGNAL_UVK_NGBSTAT_MN();


    //        }
    //    }
    foreach (auto strel, l_strel_Y) {
        strel->setSIGNAL_UVK_TLG(strel->SIGNAL_UVK_TLG().innerUse());l<< strel->SIGNAL_UVK_TLG();
        strel->setSIGNAL_UVK_NGBDYN_PL(strel->SIGNAL_UVK_NGBDYN_PL().innerUse());l<< strel->SIGNAL_UVK_NGBDYN_PL();
        strel->setSIGNAL_UVK_NGBDYN_MN(strel->SIGNAL_UVK_NGBDYN_MN().innerUse());l<< strel->SIGNAL_UVK_NGBDYN_MN();
        strel->setSIGNAL_UVK_NGBSTAT_PL(strel->SIGNAL_UVK_NGBSTAT_PL().innerUse());l<< strel->SIGNAL_UVK_NGBSTAT_PL();
        strel->setSIGNAL_UVK_NGBSTAT_MN(strel->SIGNAL_UVK_NGBSTAT_MN().innerUse());l<< strel->SIGNAL_UVK_NGBSTAT_MN();
        strel->setSIGNAL_UVK_WSTRA(strel->SIGNAL_UVK_WSTRA().innerUse());l<< strel->SIGNAL_UVK_WSTRA();
        strel->setSIGNAL_UVK_WSTRB(strel->SIGNAL_UVK_WSTRB().innerUse());l<< strel->SIGNAL_UVK_WSTRB();
        strel->setSIGNAL_UVK_ANTVZ_PL(strel->SIGNAL_UVK_ANTVZ_PL().innerUse());l<< strel->SIGNAL_UVK_ANTVZ_PL();
        strel->setSIGNAL_UVK_ANTVZ_MN(strel->SIGNAL_UVK_ANTVZ_MN().innerUse());l<< strel->SIGNAL_UVK_ANTVZ_MN();

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

    //    foreach (auto w, l_tdsopair) {
    //        if (w->strel!=nullptr){
    foreach (auto strel, l_strel_Y) {
        strel->SIGNAL_UVK_TLG().setValue_1bit(strel->STATE_UVK_TLG());
        strel->SIGNAL_UVK_NGBDYN_PL().setValue_1bit(strel->STATE_UVK_NGBDYN_PL());
        strel->SIGNAL_UVK_NGBDYN_MN().setValue_1bit(strel->STATE_UVK_NGBDYN_MN());
        strel->SIGNAL_UVK_NGBSTAT_PL().setValue_1bit(strel->STATE_UVK_NGBSTAT_PL());
        strel->SIGNAL_UVK_NGBSTAT_MN().setValue_1bit(strel->STATE_UVK_NGBSTAT_MN());
        strel->SIGNAL_UVK_WSTRA().setValue_1bit(strel->STATE_UVK_WSTRA());
        strel->SIGNAL_UVK_WSTRB().setValue_1bit(strel->STATE_UVK_WSTRB());
        strel->SIGNAL_UVK_ANTVZ_PL().setValue_1bit(strel->STATE_UVK_ANTVZ_PL());
        strel->SIGNAL_UVK_ANTVZ_MN().setValue_1bit(strel->STATE_UVK_ANTVZ_MN());
    }
}

bool tos_System_DSO::buffer2state()
{
    bool allex=true;
    foreach (auto trc, l_trc) {
        if (!trc->buffer2state()) allex=false;
    }
    return allex;
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
    // выставляем статическмй негабарит
    setNGBSTAT(T);

    // выставляем признак движения на взрез
    setANTIVZR(T);

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
    // сбросим начало конец в осях
    foreach (auto trc, l_trc) {
        for (TOtcepDataOs &os :trc->l_os){
            os.p=_pOtcepPartUnknow;
        }
    }

    TOtcepDataOs null_os;
    foreach (auto o, lo) {
        m_RC *rcs=nullptr;
        m_RC *rcf=nullptr;
        TOtcepDataOs *s_os=nullptr;
        TOtcepDataOs *f_os=nullptr;
        if (o->otcep->STATE_LOCATION()==m_Otcep::locationUnknow) continue;
        if (o->otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) continue;
        TOtcepDataOs lstOs;
        lstOs.v=_undefV_;
        foreach (auto trc, l_trc) {
            for (TOtcepDataOs &os :trc->l_os){
                if (os.num==o->otcep->NUM()){

                    if ((!rcs)||(os.os_otcep<s_os->os_otcep)){
                        rcs=trc->rc;s_os=&os;
                    }
                    if ((!rcf)||(os.os_otcep>f_os->os_otcep)){
                        rcf=trc->rc;f_os=&os;
                    }
                    if (((!lstOs.t.isValid())||(lstOs.t<os.t))&&(os.v!=_undefV_)){
                        lstOs=os;
                        if (trc->rc->STATE_BUSY_DSO_STOP()) lstOs.v=0;
                    }
                }
            }
        }
        if (s_os!=nullptr) s_os->p=_pOtcepStart;
        if (f_os!=nullptr) f_os->p=_pOtcepEnd;
        if (((s_os!=nullptr))&&(s_os==f_os)){
            s_os->p=_pOtcepStartEnd;
        }

        o->otcep->setSTATE_V_DISO(lstOs.v);

        if ((rcs==nullptr)&&(o->otcep->RCS!=nullptr)){
            resetTracking(o->otcep->NUM());
        } else {
            if ((rcs!=o->otcep->RCS) || (rcf!=o->otcep->RCF)){
                o->otcep->RCS=rcs;
                o->otcep->RCF=rcf;
                if (s_os->t>f_os->t) o->otcep->setSTATE_DIRECTION(s_os->d); else
                    o->otcep->setSTATE_DIRECTION(f_os->d);
                o->otcep->setBusyRC();

            }
        }

    }
    // проверяем на разрыв -  2 подряд свободные
    foreach (auto o, lo) {
        for (int i=0;i<o->otcep->vBusyRc.size()-2;i++){
            auto rc0=o->otcep->vBusyRc[i+0];
            auto rc1=o->otcep->vBusyRc[i+1];
            auto rc2=o->otcep->vBusyRc[i+2];
            if ((rc1->STATE_BUSY()==MVP_Enums::free)&&(rc2->STATE_BUSY()==MVP_Enums::free)){
                o->otcep->setSTATE_ERROR_TRACK(true);
                o->otcep->RCF=rc0;
                o->otcep->setBusyRC();
                // пересчитываем оси
                auto trc_rcf=mRc2TRC[o->otcep->RCF];
                if (trc_rcf!=nullptr){
                    TOtcepDataOs f_os;
                    for (TOtcepDataOs &os :trc_rcf->l_os){
                        if ((os.num==o->otcep->NUM())&&(os.os_otcep>f_os.os_otcep)) f_os=os;
                    }
                    int os_cnt=0;
                    int vag_cnt=0;
                    if (f_os.os_otcep>0){
                        os_cnt=f_os.os_otcep;
                        vag_cnt=f_os.os_otcep/4;
                        if (f_os.os_otcep%4!=0) vag_cnt++;
                    }
                    o->otcep->setSTATE_ZKR_OSY_CNT(os_cnt);
                    o->otcep->setSTATE_ZKR_VAGON_CNT(vag_cnt);
                    o->otcep->setSTATE_ZKR_TLG(vag_cnt*2);

                }
                break; //vBusyRc
            }
        }
    }
    // обезличмваем оставшиеся оси
    foreach (auto o, lo) {
        if (!o->otcep->vBusyRc.isEmpty()){
            auto num=o->otcep->NUM();
            foreach (auto trc, l_trc) {
                if (!trc->l_os.isEmpty()){
                    if (!o->otcep->vBusyRc.contains(trc->rc)){
                        trc->reset_num(num);
                        foreach (auto tzkr,l_tzkr){
                            if (tzkr->trc==trc){
                                tzkr->resetTracking(num);
                            }
                        }
                    }
                }
            }
        }
    }

    // заполняем l_otceps


    foreach (auto trc, l_trc) {
        trc->l_otceps.clear();
        for (TOtcepDataOs &os :trc->l_os){
            if (!trc->l_otceps.contains(os.num)) {
                trc->l_otceps.push_back(os.num);
            }
        }
        foreach (auto o, lo) {
            auto num=o->otcep->NUM();
            if (!trc->l_otceps.contains(num)) {
                trc->l_otceps.push_back(num);
            }

        }
    }
}

void tos_System_DSO::updateOtcepsParams(const QDateTime &)
{

    // признак на зкр
    m_Otcep *otcep_on_zkr=nullptr;
    auto azkr=modelGorka->active_zkr();
    foreach (auto o, lo) {
        auto otcep=o->otcep;
        bool inzkr=false;
        otcep->setSTATE_ZKR_S_IN(0);
        if (azkr!=nullptr){
            if (otcep->RCS==azkr){
                inzkr=true;
                otcep_on_zkr=otcep;
                otcep->setSTATE_PUT_NADVIG(azkr->PUT_NADVIG());
                otcep->setSTATE_ZKR_S_IN(1);
            }
            if (otcep->RCF==azkr){
                inzkr=true;
                otcep->setSTATE_PUT_NADVIG(azkr->PUT_NADVIG());

            }
            //дб
            if ((!inzkr)&&(otcep->STATE_ZKR_TLG()>0)&&(otcep->STATE_ZKR_TLG()%2!=0)&&
                    (otcep->RCF!=nullptr)&&(azkr!=nullptr)&&(azkr->next_rc[_forw]==otcep->RCF))
            {
                auto trc=mRc2TRC[azkr];
                if (trc->l_os.isEmpty()) inzkr=true;
            }
        }
        otcep->setSTATE_ZKR_PROGRESS(inzkr);
        int vagon_inzkr=0;
        if (inzkr){
            //            if (otcep->STATE_ZKR_TLG()%2==0){
            //                vagon_inzkr=otcep->STATE_ZKR_VAGON_CNT()+1;
            //            } else{
            //                vagon_inzkr=otcep->STATE_ZKR_VAGON_CNT();
            //            }
            vagon_inzkr=otcep->STATE_ZKR_VAGON_CNT();
        }
        for (int i=0;i<otcep->vVag.size();i++){
            auto &v=otcep->vVag[i];
            auto l=otcep->STATE_LOCATION();
            int zp=0;
            if (inzkr){
                if (i+1<=vagon_inzkr) {
                    l=m_Otcep::locationOnSpusk;
                } else {
                    l=m_Otcep::locationOnPrib;
                }
                if (i+1==vagon_inzkr) zp=1;
            }
            v.setSTATE_ZKR_PROGRESS(zp);
            v.setSTATE_LOCATION(l);
        }
    }
    if (otcep_on_zkr!=nullptr) otcep_on_zkr->setSTATE_ZKR_S_IN(1);


    foreach (auto o, lo) {
        auto otcep=o->otcep;
        if (otcep->STATE_ENABLED()) {
            if ((otcep->RCS==nullptr)&&(otcep->RCF==nullptr)&&(otcep->STATE_LOCATION()==m_Otcep::locationOnPrib)) break;
            if ((otcep->RCS==nullptr)&&(otcep->RCF==nullptr)){
                otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                continue;
            }
            int locat=m_Otcep::locationOnSpusk;

            // признак на зкр


            // KZP
            m_RC_Gor_Park * rc_park=qobject_cast<m_RC_Gor_Park *>(otcep->RCF);
            if (rc_park!=nullptr){
                locat=m_Otcep::locationOnPark;
            }


            qreal Vars=_undefV_;
            foreach (auto rc, otcep->vBusyRc) {
                if (!mRc2Zam.contains(rc)) continue;
                m_Zam *zam=mRc2Zam[rc];
                int n=zam->NTP();
                // режим торм
                if (zam->STATE_STUPEN()>0){
                    if (zam->STATE_STUPEN()>otcep->STATE_OT_RA(0,n)) otcep->setSTATE_OT_RA(0,n,zam->STATE_STUPEN());
                }
                if (zam->STATE_A()!=0){
                    otcep->setSTATE_OT_RA(1,n,zam->STATE_A());
                }
                auto ntp=zam->NTP();
                if ((ntp>=1)&&(ntp<=3)&&(zam->controllerARS()!=nullptr)) otcep->setSTATE_ADDR_TP(ntp-1,(zam->controllerARS()->ADDR_SLOT()-1)*100+zam->controllerARS()->ADDR());

                if (mRc2Ris.contains(rc)){
                    m_RIS *ris=mRc2Ris[rc];
                    if (ris->controllerARS()->isValidState()){
                        Vars=ris->STATE_V();
                        if (Vars<1.3) Vars=0;
                    }
                }

            }
            if ((qFabs(otcep->STATE_V_ARS()-Vars)>=0.4)) {
                otcep->setSTATE_V_ARS(Vars);
            }
            // маршрут
            m_RC_Gor*rc=qobject_cast<m_RC_Gor*>(otcep->RCS);
            if ((rc==nullptr)||
                    (((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)||(otcep->STATE_LOCATION()==m_Otcep::locationOnPark))&&
                     (otcep->STATE_MAR()!=0)&&
                     ((otcep->STATE_MAR()<rc->MINWAY())||(otcep->STATE_MAR()>rc->MAXWAY()))
                     )
                    )otcep->setSTATE_ERROR(true); else otcep->setSTATE_ERROR(false);
            if ((rc==nullptr)&&(rc->MINWAY()==rc->MAXWAY())&&(rc->MINWAY()!=0)) otcep->setSTATE_MAR_R(1);else otcep->setSTATE_MAR_R(0);
            int marf=0;
            while (rc!=nullptr){
                if (rc->MINWAY()==rc->MAXWAY()){
                    marf=rc->MINWAY();
                    break;
                }
                rc=qobject_cast<m_RC_Gor*>(rc->next_rc[_forw]);
            }
            otcep->setSTATE_MAR_F(marf);



            otcep->setSTATE_LOCATION(locat);

            //        // финализируем скорость отцепов
            //        if (otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk){
            //            o->updateV_RC(T);
            //        }
            otcep->setSTATE_V(o->STATE_V());

            // порядковый на рц
            int nn=0;
            foreach (auto o2, lo) {
                auto otcep2=o2->otcep;
                if ((otcep2->RCS!=nullptr)&&(otcep2->RCS==otcep->RCS)){
                    if (otcep2->NUM()>otcep->NUM()) nn++;
                }
            }
            otcep->setSTATE_D_ORDER_RC(nn);
        }

    }

}
void tos_System_DSO::resetTracking(int num)
{
    tos_System::resetTracking(num);

    foreach (auto trc, l_trc) {
        trc->reset_num(num);
    }

    foreach (auto tzkr,l_tzkr){
        if (tzkr->cur_os.num==num) tzkr->cur_os.num=0;
    }

    reset_1_os(QDateTime::currentDateTime());
}

void tos_System_DSO::resetTracking()
{
    tos_System::resetTracking();
    // сотрем оси на концевиках
    foreach (auto trc, l_trc_park) {
        trc->l_os.clear();
    }
    foreach (auto trc, l_trc) {
        if ((trc->tdso[0]==nullptr)||(trc->tdso[1]==nullptr)) {
            trc->l_os.clear();
        }
    }

    //
    foreach (auto trc, l_trc) {
        trc->l_otceps.clear();
        for (TOtcepDataOs &os :trc->l_os){
            os.num=0;
        }
        trc->rc->setSTATE_ERR_LZ(false);
        trc->rc->setSTATE_ERR_LS(false);
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
                if (rc1!=nullptr){
                    auto rc2=rc1->next_rc[d];
                    if ((rc2==nullptr)||(!rc2->l_os.isEmpty())) {nesnimat=true;break;}
                }
            }
            if (nesnimat) continue;
            auto os=trc->l_os.first();
            if (os.t.msecsTo(T)>5000){
                trc->l_os.clear();
                trc->rc->setSTATE_ERR_LZ(true);
            }
        }
    }
}

void tos_System_DSO::setDSOBUSY(const QDateTime &)
{
    foreach (auto trc, l_trc) {
        if (trc->rc->STATE_BUSY_DSO_ERR()){
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::busy);
            continue;
        }
        if (trc->l_os.isEmpty())
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::free); else
            trc->rc->setSTATE_BUSY_DSO(MVP_Enums::TRCBusy::busy);
    }
}

void tos_System_DSO::setSTATE_BUSY_DSO_ERR()
{

    foreach (auto w, l_trc) {
        w->rc->setSTATE_BUSY_DSO_ERR(true);
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

void tos_System_DSO::setANTIVZR(const QDateTime &T)
{
    foreach (auto ngb, l_NgbStrel) {
        ngb->strel->setSTATE_UVK_ANTVZ_PL(false);
        ngb->strel->setSTATE_UVK_ANTVZ_MN(false);
        for (int m=0;m<2;m++){
            foreach (auto trc, ngb->l_ngb_trc[m]) {
                if (trc->l_os.size()>=2){
                    auto os0=trc->l_os.first();
                    if (os0.d!=_back) continue;
                    auto os1=trc->l_os[1];
                    if (os1.d!=_back) continue;
                    auto ms=os0.t.msecsTo(T);
                    if (ms>2000) continue;
                    if (m==0) ngb->strel->setSTATE_UVK_ANTVZ_PL(true);
                    if (m==1) ngb->strel->setSTATE_UVK_ANTVZ_MN(true);
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
        // скорость выхода
        if ((moved_os.p==_pOtcepEnd)||(moved_os.p==_pOtcepStartEnd)){
            if (rc1!=nullptr) setOtcepVIO(1,rc1, moved_os);
        }
        // скорость входа
        if (moved_os.os_otcep==2){
            if (rc0!=nullptr) setOtcepVIO(0,rc0, moved_os);
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
    foreach (auto strel, l_strel_Y) {
        strel->setSTATE_UVK_WSTRA(false);
        strel->setSTATE_UVK_WSTRB(false);
    }

    auto act_zkr=modelGorka->active_zkr();
    foreach (auto od, lo) {
        auto otcep=od->otcep;
        int warn1=0;
        int warn2=0;
        if (!otcep->STATE_ENABLED()) continue;
        if (act_zkr!=nullptr){
            if ((otcep->STATE_DIRECTION()==_forw)&&
                    (otcep->STATE_MAR()>0)&&
                    ((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)||(otcep->STATE_LOCATION()==m_Otcep::locationOnPrib))) {
                auto m=modelGorka->getMarshrut(act_zkr->PUT_NADVIG(),otcep->STATE_MAR());
                if (m==nullptr) continue;
                // идем по маршруту
                bool begin_found=false;
                bool ex_busy=false;
                bool ex_pred_busy=false;
                if (otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) begin_found=true;
                for (const RcInMarsrut&mr :m->l_rc){
                    // идем по маршруту с головы отцепа
                    if (!begin_found){
                        if (mr.rc==otcep->RCS) begin_found=true;
                        continue;
                    }
                    if (mr.rc->STATE_BUSY()==MVP_Enums::busy) ex_busy=true;
                    if (warn1==0){

                        auto str1=qobject_cast<m_Strel_Gor*>(mr.rc) ;
                        if (str1!=nullptr){
                            bool bw=false;
                            if (str1->STATE_POL()!=mr.pol){
                                bw=true;
                                // стрелки в среднем положении
                                m_Strel_Gor_Y* str=qobject_cast<m_Strel_Gor_Y*>(str1) ;
                                if (str!=nullptr){
                                    if (str->STATE_A()==1){
                                        bw=false;
                                        if ((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)&&
                                                ((otcep->STATE_GAC_ACTIVE()==0)||(modelGorka->STATE_REGIM()!=ModelGroupGorka::regimRospusk))){
                                            bw=true;
                                        }
                                    }
                                    if (bw) {
                                        if ((!ex_busy)&&((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk) || (otcep->STATE_IS_CURRENT()==1)))
                                            str->setSTATE_UVK_WSTRA(true);
                                    }
                                }
                            }
                            if (bw) warn1=1;
                        }
                    }
                    if (warn2==0){
                        bool bw1=false;
                        bool bw2=false;
                        // что то стоит на пути
                        if ((mr.rc->MINWAY()!=mr.rc->MAXWAY())&&
                                ((mr.rc->STATE_BUSY_DSO_ERR())||(mr.rc->STATE_BUSY_DSO_STOP()))
                                ){
                            bw1=true;

                        }

                        m_Strel_Gor_Y* str=qobject_cast<m_Strel_Gor_Y*>(mr.rc) ;
                        if (str!=nullptr){
                            // негабаритность
                            if ((str->STATE_UVK_NGBDYN_PL()==1) && (mr.pol==MVP_Enums::pol_minus)) bw2=true;
                            if ((str->STATE_UVK_NGBDYN_MN()==1) && (mr.pol==MVP_Enums::pol_plus))  bw2=true;
                            if ((str->STATE_UVK_NGBSTAT_PL()==1) && (mr.pol==MVP_Enums::pol_minus)) bw2=true;
                            if ((str->STATE_UVK_NGBSTAT_MN()==1) && (mr.pol==MVP_Enums::pol_plus))  bw2=true;
                            // АВ
                            if (str->STATE_POL()!=mr.pol){
                                if (str->STATE_UVK_AV()) bw2=true;
                            }
                            if ((bw1)||(bw2)) warn2=1;
                            if (bw1) {
                                if ((!ex_pred_busy)&&((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk) || (otcep->STATE_IS_CURRENT()==1)))
                                    str->setSTATE_UVK_WSTRB(true);
                            }
                            if (bw2) {
                                if ((!ex_busy)&&((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk) || (otcep->STATE_IS_CURRENT()==1)))
                                    str->setSTATE_UVK_WSTRB(true);
                            }
                        }

                    }
                    ex_pred_busy=ex_busy;
                }

            }
        }
        otcep->setSTATE_GAC_W_STRA(warn1);
        otcep->setSTATE_GAC_W_STRB(warn2);
    }
}

void tos_System_DSO::setOtcepVIO(int io,tos_Rc *trc, TOtcepDataOs os)
{
    if (os.v==_undefV_) return;
    auto o=otcep_by_num(os.num);
    if (o!=nullptr){
        // скорость входа
        if ((io==0)&&(os.os_otcep==2)){
            if (mRc2Zam.contains(trc->rc)){
                m_Zam *zam=mRc2Zam[trc->rc];
                int n=zam->NTP()-1;
                if ((zam->TIPZM()!=2)) o->otcep->setSTATE_V_INOUT(0,n,os.v);
            }
        }
        // скорость выхода
        if ((io==1)&&(os.os_otcep>=2)){
            if (mRc2Zam.contains(trc->rc)){
                m_Zam *zam=mRc2Zam[trc->rc];
                int n=zam->NTP()-1;
                if ((zam->TIPZM()>0)) o->otcep->setSTATE_V_INOUT(1,n,os.v);
            }
        }
    }
}
