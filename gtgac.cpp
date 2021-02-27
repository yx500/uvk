#include "gtgac.h"




GtGac::GtGac(QObject *parent, ModelGroupGorka *modelGorka):BaseWorker(parent)
{
    this->modelGorka=modelGorka;
    QList<m_Strel_Gor_Y*> l_strel_Y=modelGorka->findChildren<m_Strel_Gor_Y*>();
    foreach (m_Strel_Gor_Y*strel, l_strel_Y) {
        GacStrel *gstr=new GacStrel;
        gstr->strel=strel;

        auto s=strel->SIGNAL_UVK_AV();
        if (!s.isEmpty()){
            gstr->SIGNAL_UVK_ERR_PLATA=     SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+1).innerUse();
            gstr->SIGNAL_UVK_ERR_PER  =     SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+2).innerUse();
            gstr->SIGNAL_UVK_PRP_MAR  =     SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+3).innerUse();
            gstr->SIGNAL_UVK_PRM_MAR  =     SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+4).innerUse();
            gstr->SIGNAL_UVK_BL_PER_DONE  = SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+5).innerUse();

        }

        s=strel->SIGNAL_UVK_BL_PER();
        if (!s.isEmpty()){
            gstr->SIGNAL_UVK_BL_PER_SP=     SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+1).innerUse();
            gstr->SIGNAL_UVK_BL_PER_DB=     SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+2).innerUse();
            gstr->SIGNAL_UVK_BL_PER_OTC=    SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+3).innerUse();
            gstr->SIGNAL_UVK_BL_PER_TLG=    SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+4).innerUse();
            gstr->SIGNAL_UVK_BL_PER_NGBSTAT=SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+5).innerUse();
            gstr->SIGNAL_UVK_BL_PER_NGBDYN= SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+6).innerUse();
            gstr->SIGNAL_UVK_BL_SRPOL_TO=   SignalDescription(s.chanelType(),s.chanelName(),s.chanelOffset()+7).innerUse();
        }


        l_strel.push_back(gstr);
        mRC2GS[strel]=gstr;
    }
    auto l_otceps=modelGorka->findChildren<m_Otceps*>();
    otceps=l_otceps.first();
    l_zkr=modelGorka->findChildren<m_RC_Gor_ZKR*>();



}

QList<SignalDescription> GtGac::acceptOutputSignals()
{
    QList<SignalDescription> l;
    foreach (auto gstr ,l_strel){
        gstr->strel->setSIGNAL_UVK_PRP(gstr->strel->SIGNAL_UVK_PRP().innerUse());   l << gstr->strel->SIGNAL_UVK_PRP();
        gstr->strel->setSIGNAL_UVK_PRM(gstr->strel->SIGNAL_UVK_PRM().innerUse());   l << gstr->strel->SIGNAL_UVK_PRM();
        gstr->strel->setSIGNAL_UVK_AV(gstr->strel->SIGNAL_UVK_AV().innerUse());     l << gstr->strel->SIGNAL_UVK_AV();
        gstr->strel->setTU_PRP(gstr->strel->TU_PRP().innerUse());                   l << gstr->strel->TU_PRP();
        gstr->strel->setTU_PRM(gstr->strel->TU_PRM().innerUse());                   l << gstr->strel->TU_PRM();
        gstr->strel->setSIGNAL_UVK_BL_PER(gstr->strel->SIGNAL_UVK_BL_PER().innerUse());         l << gstr->strel->SIGNAL_UVK_BL_PER();
        l << gstr->SIGNAL_UVK_ERR_PLATA<<
             gstr->SIGNAL_UVK_ERR_PER<<
             gstr->SIGNAL_UVK_PRP_MAR<<
             gstr->SIGNAL_UVK_PRM_MAR<<
             gstr->SIGNAL_UVK_BL_PER_DONE;


        l << gstr->SIGNAL_UVK_BL_PER_SP <<
             gstr->SIGNAL_UVK_BL_PER_DB <<
             gstr->SIGNAL_UVK_BL_PER_OTC <<
             gstr->SIGNAL_UVK_BL_PER_TLG <<
             gstr->SIGNAL_UVK_BL_PER_NGBSTAT <<
             gstr->SIGNAL_UVK_BL_PER_NGBDYN<<
             gstr->SIGNAL_UVK_BL_SRPOL_TO;

    }
    return l;

}

void GtGac::state2buffer()
{
    foreach (auto gs ,l_strel){
        gs->strel->SIGNAL_UVK_PRP().setValue_1bit(gs->strel->STATE_UVK_PRP());
        gs->strel->SIGNAL_UVK_PRM().setValue_1bit(gs->strel->STATE_UVK_PRM());
        gs->strel->SIGNAL_UVK_AV().setValue_1bit(gs->strel->STATE_UVK_AV());

        gs->SIGNAL_UVK_ERR_PLATA.setValue_1bit(gs->ERR_PLATA);
        gs->SIGNAL_UVK_ERR_PER.setValue_1bit(gs->ERR_PER);

        bool pol_plus=false;
        bool pol_minus=false;
        if (gs->pol_mar==MVP_Enums::pol_plus) pol_plus=true;
        if (gs->pol_mar==MVP_Enums::pol_minus) pol_minus=true;
        gs->SIGNAL_UVK_PRP_MAR.setValue_1bit(pol_plus);
        gs->SIGNAL_UVK_PRM_MAR.setValue_1bit(pol_minus);
        gs->SIGNAL_UVK_BL_PER_DONE.setValue_1bit(gs->BL_PER_DONE);




        gs->strel->SIGNAL_UVK_BL_PER().setValue_1bit(gs->BL_PER);

        gs->SIGNAL_UVK_BL_PER_SP.setValue_1bit(gs->BL_PER_SP);
        gs->SIGNAL_UVK_BL_PER_DB.setValue_1bit(gs->BL_PER_DB);
        gs->SIGNAL_UVK_BL_PER_OTC.setValue_1bit(gs->BL_PER_OTC);
        gs->SIGNAL_UVK_BL_PER_TLG.setValue_1bit(gs->BL_PER_TLG);
        gs->SIGNAL_UVK_BL_PER_NGBSTAT.setValue_1bit(gs->BL_PER_NGBSTAT);
        gs->SIGNAL_UVK_BL_PER_NGBDYN.setValue_1bit(gs->BL_PER_NGBDYN);
        gs->SIGNAL_UVK_BL_SRPOL_TO.setValue_1bit(gs->BL_SRPOL_TO);
    }
}





void GtGac::resetStates()
{
    foreach (auto gs ,l_strel){
        gs->ERR_PLATA=false;
        gs->ERR_PER=false;
        gs->sred_pol_time=QDateTime();
        gs->pol_cmd_w_time=QDateTime();
        gs->BL_PER_DONE=false;
        gs->pol_zad=MVP_Enums::pol_unknow;
        sendCommand(gs,MVP_Enums::pol_unknow,true);
    }

}

bool collectTree(m_RC * rc,QList<m_RC_Gor*> &l)
{
    if (l.size()>1000) return false;
    for (int m=0;m<rc->getNextCount(0);m++){
        m_RC_Gor * nrc=qobject_cast<m_RC_Gor*>(rc->getNextRC(0,m));
        if (nrc!=nullptr){
            if (l.indexOf(nrc)>=0) return false;
            l.push_back(nrc);
            if (!collectTree(nrc,l)) return false;
        }
    }
    return true;
}

void GtGac::validation(ListObjStr *l) const
{
    for (auto gs : l_strel){
        if (gs->strel->MINWAY()==0) l->error(gs->strel,"MINWAY=0");
        if (gs->strel->MAXWAY()==0) l->error(gs->strel,"MAXWAY=0");

        if (gs->strel->TU_PRP().isEmpty()) l->error(gs->strel,"TU_PRP=0");else
            if (gs->strel->TU_PRM().isEmpty()) l->error(gs->strel,"TU_PRM=0");else
                if (gs->strel->TU_PRP()==gs->strel->TU_PRM()) l->error(gs->strel,"TU_PRM=TU_PRP");
        if (gs->strel->SIGNAL_PRP().isEmpty()) l->error(gs->strel,"SIGNAL_PRP empty");
        if (gs->strel->SIGNAL_PRM().isEmpty()) l->error(gs->strel,"SIGNAL_PRM empty");
        if (gs->strel->SIGNAL_UVK_AV().isEmpty()) l->warning(gs->strel,"SIGNAL_AV empty");
        if (gs->strel->SIGNAL_UVK_PRM().isEmpty()) l->warning(gs->strel,"SIGNAL_UVK_PRM empty");
        if (gs->strel->SIGNAL_UVK_PRP().isEmpty()) l->warning(gs->strel,"SIGNAL_UVK_PRP empty");
    }
    for (auto gs1 : l_strel){
        for (auto gs2 : l_strel){
            if (gs1->strel==gs2->strel) continue;
            if (gs1->strel->TU_PRP()==gs2->strel->TU_PRP()) l->error(gs1->strel,QString("TU_PRP=%1 TU_PRP").arg(gs2->strel->objectName()));
            if (gs1->strel->TU_PRP()==gs2->strel->TU_PRM()) l->error(gs1->strel,QString("TU_PRP=%1 TU_PRM").arg(gs2->strel->objectName()));
            if (gs1->strel->TU_PRM()==gs2->strel->TU_PRP()) l->error(gs1->strel,QString("TU_PRM=%1 TU_PRP").arg(gs2->strel->objectName()));
            if (gs1->strel->TU_PRM()==gs2->strel->TU_PRM()) l->error(gs1->strel,QString("TU_PRM=%1 TU_PRM").arg(gs2->strel->objectName()));
        }
    }
    for (auto gs : l_strel){
        QList<m_RC_Gor*> lrc;
        lrc.clear();
        if (!collectTree(gs->strel,lrc)) {
            QString last="";
            if (!lrc.isEmpty()) last=lrc.last()->objectName();
            l->error(gs->strel,QString("Не построить дерево маршрутов "+last));
        } else {
            foreach (m_RC_Gor *rc, lrc) {
                if (rc->MINWAY()<gs->strel->MINWAY()) l->error(rc,QString("MINWAY вне диапазона %1->%2").arg(gs->strel->objectName()).arg(gs->strel->MINWAY()));
                if (rc->MAXWAY()>gs->strel->MAXWAY()) l->error(rc,QString("MAXWAY вне диапазона %1->%2").arg(gs->strel->objectName()).arg(gs->strel->MAXWAY()));

            }

            for(int mar=gs->strel->MINWAY();mar<=gs->strel->MAXWAY();mar++){
                bool exmar=false;
                foreach (m_RC_Gor *rc, lrc) {
                    if ((rc->MINWAY()==rc->MAXWAY())&&(mar==rc->MINWAY())){
                        exmar=true;
                        break;
                    }
                }
                if (!exmar){
                    l->error(gs->strel,QString("Нет реализации для mar=%1").arg(mar));
                }
            }
        }
    }
}





void GtGac::set_STATE_GAC_ACTIVE()
{
    if (!FSTATE_ENABLED) {
        foreach (m_Otcep *otcep, otceps->otceps())   otcep->setSTATE_GAC_ACTIVE(0);
        return;
    }

    // включаем  1 отцеп
    auto otcep1=otceps->topOtcep();
    foreach (m_Otcep *otcep, otceps->otceps()) {
        if (otcep->STATE_ENABLED()){
            if (otcep->STATE_LOCATION()==m_Otcep::locationOnPrib){
                if (otcep==otcep1)
                    otcep->setSTATE_GAC_ACTIVE(1); else
                    otcep->setSTATE_GAC_ACTIVE(0);
            }
        }else  otcep->setSTATE_GAC_ACTIVE(0);
    }

    // выключем отцепы
    foreach (m_Otcep *otcep, otceps->otceps()) {
        if (!otcep->STATE_ENABLED()) continue;
        if (!otcep->STATE_GAC_ACTIVE()) continue;
        int act=1;

        // не задан
        if (otcep->STATE_MAR()==0) act=0;

        // для первого выставляем даж когда он еще не выехал
        if (otcep==otcep1){
            if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib) act=0;
        } else {
            if (otcep->STATE_LOCATION()!=m_Otcep::locationOnSpusk) act=0;
        }

        // не выставляем для отцепов которые едут в гору
        if (otcep->STATE_DIRECTION()!=_forw) act=0;

        if (otcep->STATE_ERROR()!=0) act=0;

        // реализован
        auto rcs=qobject_cast<m_RC_Gor*>(otcep->RCS);
        if ((rcs!=nullptr)&&(rcs->MINWAY()==rcs->MAXWAY())) act=0;
        if (act!=otcep->STATE_GAC_ACTIVE()){
            otcep->setSTATE_GAC_ACTIVE(act);
        }


    }



}

void GtGac::work(const QDateTime &T)
{

    // выставляем блокировку перевода
    foreach (auto gs ,l_strel){
        setStateBlockPerevod(gs,T);
    }

    foreach (auto gs ,l_strel){
        gs->pol_zad=MVP_Enums::pol_unknow;
        gs->pol_mar=MVP_Enums::pol_unknow;

        if (gs->strel->STATE_A()==1) {
            // выставляем время ср
            if (!gs->sred_pol_time.isValid()) gs->sred_pol_time=T;
        } else {
            gs->sred_pol_time=QDateTime();
            // убираем автовозврат
            gs->strel->setSTATE_UVK_AV(false);
            gs->ERR_PER=false;
            gs->BL_PER_DONE=false;
        }
        // снимаем признак отхлопывания
        if (gs->BL_PER_DONE) {
            if (gs->pol_UVK_BL_PER_DONE!=gs->strel->STATE_POL())gs->BL_PER_DONE=false;
        }
    }

    //   1 отцеп
    auto otcep1=otceps->topOtcep();

    // выключем отцепы
    set_STATE_GAC_ACTIVE();

    auto act_zkr=modelGorka->active_zkr();
    if (FSTATE_ENABLED) {
        // определяем на стрелках нужное для отцепов положение pol_mar
        foreach (m_Otcep *otcep, otceps->otceps()) {
            if (!otcep->STATE_ENABLED()) continue;
            if ((modelGorka->STATE_REGIM()!=ModelGroupGorka::regimRospusk)&&
                    (modelGorka->STATE_REGIM()!=ModelGroupGorka::regimPausa)) continue;

            if (!otcep->STATE_GAC_ACTIVE()) continue;

            auto rcs=qobject_cast<m_RC_Gor*>(otcep->RCS);
            if (otcep==otcep1){
                rcs=nullptr;
                if ((act_zkr!=nullptr) &&
                        (act_zkr->STATE_BUSY()==MVP_Enums::free)&&
                        (otceps->otcepCountOnRc(act_zkr)==0)
                        ) rcs=act_zkr;
            }

            if (rcs==nullptr) continue;
            // 1 ось - первая на рц, т.е. если есть на рц отцепы с меньшим номером то пропускаем
            if (otceps->otcepCountOnRc(rcs)>1){
                bool ex_min=false;
                foreach (auto o1, otceps->otcepsOnRc(rcs)) {
                    if (o1->NUM()<otcep->NUM()) ex_min=true;
                }
                if (ex_min) continue;
            }

            // на случай если хвост вышел из зоны ГАЦ
            auto rcf=qobject_cast<m_RC_Gor*>(otcep->RCF);
            if (rcf!=nullptr) {
                if (rcf->MINWAY()==rcf->MAXWAY()) continue;
            }

            // были случаи когда затаскивали отцеп на другую гору
            foreach (auto rc_zkr, l_zkr) {
                if ((rc_zkr==rcs) && (rc_zkr->STATE_ROSPUSK()!=1)) continue;
            }

            auto rc=rcs->next_rc[0];
            int recurs_count=0;
            while(rc!=nullptr){
                recurs_count++;
                if (recurs_count>200) break; // защита от бесконечного цикла при сбойном графе
                // до первой занятости
                if (rc->STATE_BUSY()!=MVP_Enums::free) break;
                // до первой занятости отцепом
                if (otceps->otcepCountOnRc(rc)!=0) break;

                auto grc=qobject_cast<m_RC_Gor*>(rc);
                // дальше считаем стрелок нет
                if (grc->MINWAY()==grc->MAXWAY()) break;

                if (grc->MINWAY()>otcep->STATE_MAR()) break;
                if (grc->MAXWAY()<otcep->STATE_MAR()) break;
                // наша стрелка?
                if (mRC2GS.contains(rc)){
                    GacStrel*gs=mRC2GS[rc];
                    // только первый отцеп решает
                    // но по сути это ситуация когда из 2 точек есть выход на одну рц
                    if (gs->pol_mar!=MVP_Enums::pol_unknow) break;
                    bool mar_plus=false;
                    bool mar_mnus=false;
                    auto rcplus=qobject_cast<m_RC_Gor*>(gs->strel->getNextRC(0,0));
                    auto rcmnus=qobject_cast<m_RC_Gor*>(gs->strel->getNextRC(0,1));
                    if (rcplus!=nullptr) {
                        if (inway(otcep->STATE_MAR(),rcplus->MINWAY(),rcplus->MAXWAY())) mar_plus=true;
                    }
                    if (rcmnus!=nullptr) {
                        if (inway(otcep->STATE_MAR(),rcmnus->MINWAY(),rcmnus->MAXWAY())) mar_mnus=true;
                    }
                    if (mar_plus==mar_mnus) break;
                    if (mar_plus) gs->pol_mar=MVP_Enums::pol_plus;
                    if (mar_mnus) gs->pol_mar=MVP_Enums::pol_minus;
                    // до первой необходимости перевода
                    if ((gs->strel->STATE_POL()!=gs->pol_mar)) break;
                }
                // next_rc уже выставвлен по положению стрелки
                rc=rc->next_rc[_forw];
            }
        }
    }

    // определяем возможность команды на перевод
    foreach (auto gs ,l_strel){
        if (gs->strel->STATE_A()!=1) continue;
        if (gs->BL_PER) continue;

        // перевод при блокировке
        if ((gs->strel->STATE_UVK_NGBSTAT_PL())&&(gs->strel->STATE_POL()!=MVP_Enums::pol_plus)){
            gs->pol_zad=MVP_Enums::pol_plus;
            gs->pol_UVK_BL_PER_DONE=gs->pol_zad;
            gs->BL_PER_DONE=true;
            continue;
        }
        // перевод при блокировке
        if ((gs->strel->STATE_UVK_NGBSTAT_MN())&&(gs->strel->STATE_POL()!=MVP_Enums::pol_minus)){
            gs->pol_zad=MVP_Enums::pol_minus;
            gs->pol_UVK_BL_PER_DONE=gs->pol_zad;
            gs->BL_PER_DONE=true;
            continue;
        }
        // перевод по маршруту
        if (gs->pol_mar==MVP_Enums::pol_unknow) continue;
        if (gs->strel->STATE_POL()==gs->pol_mar) continue;
        gs->pol_zad=gs->pol_mar;
    }

    // задаем команды
    foreach (auto gs ,l_strel){
        sendCommand(gs,gs->pol_zad);
    }

    // проверки времени перевода
    foreach (auto gs ,l_strel){
        if (gs->pol_cmd!=MVP_Enums::pol_unknow){
            // определям начало перевода по команде
            if (gs->strel->STATE_POL()==MVP_Enums::pol_w){
                // начало потери контроля
                if (!gs->pol_cmd_w_time.isValid()) gs->pol_cmd_w_time=T;
            }
        }
    }
    // определям автовозврат
    foreach (auto gs ,l_strel){
        if (gs->pol_cmd!=MVP_Enums::pol_unknow){
            if (gs->pol_cmd_w_time.isValid()){
                if (gs->strel->STATE_POL()!=MVP_Enums::pol_w){
                    if (gs->pol_cmd!=gs->strel->STATE_POL()){
                        qint64 ms=gs->pol_cmd_w_time.msecsTo(T);
                        if ((ms>100) &&(ms<2000)) {
                            gs->strel->setSTATE_UVK_AV(true);
                        }
                    } else {
                        // перевелась таки
                        gs->ERR_PER=false;
                    }
                }

            }
        }
    }
    // определям общий неперевод
    foreach (auto gs ,l_strel){
        if (gs->pol_cmd==MVP_Enums::pol_unknow) continue;
        if (gs->pol_cmd!=gs->strel->STATE_POL()){
            if ((gs->pol_cmd_time.isValid())&&(gs->pol_cmd_time.elapsed()>=5000)){
                gs->ERR_PER=true;
            }
        }

    }

    //сбраcываем таймер
    foreach (auto gs ,l_strel){
        if (gs->strel->STATE_POL()!=MVP_Enums::pol_w) gs->pol_cmd_w_time=QDateTime();
    }

}


void GtGac::setStateBlockPerevod(GacStrel *gs,const QDateTime &T)
{
    if (otceps->otcepOnRc(gs->strel)!=nullptr) gs->BL_PER_OTC=true;else gs->BL_PER_OTC=false;
    bool sp=false;
    if ((gs->strel->get_rtds()!=nullptr)&&(gs->strel->get_rtds()->STATE_SRAB()==1)) sp=true;
    if ((gs->strel->get_ipd()!=nullptr)&&(gs->strel->get_ipd()->STATE_SRAB()==1)) sp=true;
    if (gs->strel->STATE_BUSY()!=0) sp=true;
    if (gs->strel->STATE_BUSY_DSO()!=0) sp=true;
    if (gs->strel->STATE_BUSY_DSO_ERR()!=0) sp=true;
    gs->BL_PER_SP=sp;

    gs->BL_PER_DB=false;  // Как определить дб?

    gs->BL_PER_TLG=gs->strel->STATE_UVK_TLG();

    gs->BL_PER_NGBDYN=false;
    gs->BL_PER_NGBSTAT=false;
    // yнельзя переод чтоб оставлся негабарит
    if ((gs->strel->STATE_UVK_NGBDYN_PL())&&(gs->strel->STATE_POL()==MVP_Enums::pol_plus)) gs->BL_PER_NGBDYN=true;
    if ((gs->strel->STATE_UVK_NGBDYN_MN())&&(gs->strel->STATE_POL()==MVP_Enums::pol_minus)) gs->BL_PER_NGBDYN=true;
    if ((gs->strel->STATE_UVK_NGBSTAT_PL())&&(gs->strel->STATE_POL()==MVP_Enums::pol_plus)) gs->BL_PER_NGBSTAT=true;
    if ((gs->strel->STATE_UVK_NGBSTAT_MN())&&(gs->strel->STATE_POL()==MVP_Enums::pol_minus)) gs->BL_PER_NGBSTAT=true;

    // задержка на выставление ссреднего положения стрелки
    gs->BL_SRPOL_TO=false;
    if (gs->strel->STATE_A()==1) {
        if ((!gs->sred_pol_time.isValid())||(gs->sred_pol_time.msecsTo(T)<500)) gs->BL_SRPOL_TO=true;
    }

    gs->BL_PER=gs->BL_PER_OTC |
            gs->BL_PER_SP |
            gs->BL_PER_DB |
            gs->BL_PER_NGBSTAT |
            gs->BL_PER_NGBDYN |
            gs->BL_SRPOL_TO |
            gs->strel->STATE_UVK_AV();



    //gs->BL_PER=gs->BL_PER |gs->BL_PER_TLG ;

}


void GtGac::sendCommand(GacStrel * gs, MVP_Enums::TStrelPol pol_cmd,bool force)
{
    if ((gs->pol_cmd!=pol_cmd)||(force)){
        gs->pol_cmd=pol_cmd;
        gs->pol_cmd_time.start();
        if (gs->pol_cmd==MVP_Enums::pol_plus) {
            emit uvk_command(gs->strel->TU_PRP(),1);
            emit uvk_command(gs->strel->TU_PRM(),0);
            gs->strel->setSTATE_UVK_PRP(true);
            gs->strel->setSTATE_UVK_PRM(false);
            gs->emit_time.restart();

        }
        if (gs->pol_cmd==MVP_Enums::pol_minus) {
            emit uvk_command(gs->strel->TU_PRP(),0);
            emit uvk_command(gs->strel->TU_PRM(),1);
            gs->strel->setSTATE_UVK_PRP(false);
            gs->strel->setSTATE_UVK_PRM(true);
            gs->emit_time.restart();
        }
        if (gs->pol_cmd==MVP_Enums::pol_unknow) {
            emit uvk_command(gs->strel->TU_PRP(),0);
            emit uvk_command(gs->strel->TU_PRM(),0);
            gs->strel->setSTATE_UVK_PRP(false);
            gs->strel->setSTATE_UVK_PRM(false);
            gs->emit_time.restart();
        }
    } else {
        // если нет ответа от платы шлем еще
        if (((gs->strel->STATE_UVK_PRP()!=gs->strel->STATE_PRP()))||
                ((gs->strel->STATE_UVK_PRM()!=gs->strel->STATE_PRM()))){
            // не забиваем сеть
            if ((gs->pol_cmd_time.isValid())||(gs->emit_time.elapsed()>20)){
                emit uvk_command(gs->strel->TU_PRP(),gs->strel->STATE_UVK_PRP());
                emit uvk_command(gs->strel->TU_PRM(),gs->strel->STATE_UVK_PRM());
                gs->emit_time.restart();
            }
            // нет ответа от платы
            if ((gs->pol_cmd_time.isValid()) && (gs->pol_cmd_time.elapsed()>500)){
                gs->ERR_PLATA=true;
                // не отключаем команду
            }
        } else {
            gs->ERR_PLATA=false;
        }

    }
}


