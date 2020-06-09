#include "gtgac.h"

GtGac::GtGac(QObject *parent,TrackingOtcepSystem *TOS):BaseWorker(parent)
{


    this->TOS=TOS;
    l_strel=TOS->modelGorka->findChildren<m_Strel_Gor_Y*>();

}

QList<SignalDescription> GtGac::acceptOutputSignals()
{
    QList<SignalDescription> l;
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        strel->setSIGNAL_UVK_PRP(strel->SIGNAL_UVK_PRP().innerUse());
        strel->setSIGNAL_UVK_PRM(strel->SIGNAL_UVK_PRM().innerUse());
        strel->setSIGNAL_UVK_PRM(strel->SIGNAL_UVK_AV().innerUse());
        strel->setTU_PRP(strel->TU_PRP().innerUse());
        strel->setTU_PRM(strel->TU_PRM().innerUse());
        l << strel->SIGNAL_UVK_PRP() << strel->SIGNAL_UVK_PRM() << strel->SIGNAL_UVK_AV();
    }
    return l;

}

void GtGac::resetStates()
{
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        strel->rcs->pol_zad=MVP_Enums::pol_unknow;
        sendCommand(strel,MVP_Enums::pol_unknow,true);
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
    foreach (m_Strel_Gor_Y * strel, l_strel) {
        if (strel->MINWAY()==0) l->error(strel,"MINWAY=0");
        if (strel->MAXWAY()==0) l->error(strel,"MAXWAY=0");

        if (strel->TU_PRP().isEmpty()) l->error(strel,"TU_PRP=0");else
            if (strel->TU_PRM().isEmpty()) l->error(strel,"TU_PRM=0");else
                if (strel->TU_PRP()==strel->TU_PRM()) l->error(strel,"TU_PRM=TU_PRP");
        if (strel->SIGNAL_PRP().isEmpty()) l->error(strel,"SIGNAL_PRP empty");
        if (strel->SIGNAL_PRM().isEmpty()) l->error(strel,"SIGNAL_PRM empty");
        if (strel->SIGNAL_UVK_AV().isEmpty()) l->warning(strel,"SIGNAL_AV empty");
        if (strel->SIGNAL_UVK_PRM().isEmpty()) l->warning(strel,"SIGNAL_UVK_PRM empty");
        if (strel->SIGNAL_UVK_PRP().isEmpty()) l->warning(strel,"SIGNAL_UVK_PRP empty");
    }
    foreach (m_Strel_Gor_Y * strel1, l_strel) {
        foreach (m_Strel_Gor_Y * strel2, l_strel) {
            if (strel1==strel2) continue;
            if (strel1->TU_PRP()==strel2->TU_PRP()) l->error(strel1,QString("TU_PRP=%1 TU_PRP").arg(strel2->objectName()));
            if (strel1->TU_PRP()==strel2->TU_PRM()) l->error(strel1,QString("TU_PRP=%1 TU_PRM").arg(strel2->objectName()));
            if (strel1->TU_PRM()==strel2->TU_PRP()) l->error(strel1,QString("TU_PRM=%1 TU_PRP").arg(strel2->objectName()));
            if (strel1->TU_PRM()==strel2->TU_PRM()) l->error(strel1,QString("TU_PRM=%1 TU_PRM").arg(strel2->objectName()));
        }
    }
    foreach (m_Strel_Gor_Y * strel, l_strel) {
        QList<m_RC_Gor*> lrc;
        lrc.clear();
        if (!collectTree(strel,lrc)) {
            QString last="";
            if (!lrc.isEmpty()) last=lrc.last()->objectName();
            l->error(strel,QString("Не построить дерево маршрутов "+last));
        } else {
            foreach (m_RC_Gor *rc, lrc) {
                if (rc->MINWAY()<strel->MINWAY()) l->error(rc,QString("MINWAY вне диапазона %1->%2").arg(strel->objectName()).arg(strel->MINWAY()));
                if (rc->MAXWAY()>strel->MAXWAY()) l->error(rc,QString("MAXWAY вне диапазона %1->%2").arg(strel->objectName()).arg(strel->MAXWAY()));

            }

            for(int mar=strel->MINWAY();mar<=strel->MAXWAY();mar++){
                bool exmar=false;
                foreach (m_RC_Gor *rc, lrc) {
                    if ((rc->MINWAY()==rc->MAXWAY())&&(mar==rc->MINWAY())){
                        exmar=true;
                        break;
                    }
                }
                if (!exmar){
                    l->error(strel,QString("Нет реализации для mar=%1").arg(mar));
                }
            }

        }
    }

}



bool isNegabarit(m_Strel_Gor_Y*strel,MVP_Enums::TStrelPol pol)
{
    if (pol==MVP_Enums::pol_plus){
        if (strel->NEGAB_RC_CNT_M()>0){
            m_RC* rc=strel->getNextRC(0,1);
            for(int i=0;i<strel->NEGAB_RC_CNT_M();i++){
                if (rc==nullptr) return false;
                if (rc->STATE_BUSY()==1) return true;
                rc=rc->getNextRC(0,0);
            }
        }
    }
    if (pol==MVP_Enums::pol_minus){
        if (strel->NEGAB_RC_CNT_P()>0){
            m_RC* rc=strel->getNextRC(0,0);
            for(int i=0;i<strel->NEGAB_RC_CNT_P();i++){
                if (rc==nullptr) return false;
                if (rc->STATE_BUSY()==1) return true;
                rc=rc->getNextRC(0,0);
            }
        }
    }
    return false;
}

void GtGac::work(const QDateTime &T)
{
    if (!FSTATE_ENABLED) return;
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        strel->rcs->pol_zad=MVP_Enums::pol_unknow;
        strel->rcs->pol_mar=MVP_Enums::pol_unknow;
        // убираем автовозврат
        if (strel->STATE_A()!=1) strel->setSTATE_UVK_AV(false);
    }
    // определяем на стрелках нужное для отцепов положение pol_mar
    foreach (m_Otcep *otcep, TOS->lo) {
        if (!otcep->STATE_ENABLED()) continue;
        if ((otcep->NUM()==1) && (otcep->STATE_LOCATION()==m_Otcep::locationOnPrib)){
            // для 1 первого выставляем даж когда он еще не выехал
        } else {
            if (otcep->STATE_LOCATION()!=m_Otcep::locationOnSpusk) continue;
            // не выставляем для отцепов которые едут в гору
            if (otcep->STATE_DIRECTION()!=0) continue;
        }
        auto rcs=qobject_cast<m_RC_Gor*>(otcep->RCS);

        if (rcs==nullptr) continue;
        // реализован
        if (rcs->MINWAY()==rcs->MAXWAY()) continue;
        // на случай если хвост вышел из зоны ГАЦ
        auto rcf=qobject_cast<m_RC_Gor*>(otcep->RCF);
        if (rcf==nullptr) continue;
        if (rcf->MINWAY()==rcf->MAXWAY()) continue;
        // были случаи когда затаскивали отцеп на другую гору
        auto rc_zkr=qobject_cast<m_RC_Gor_ZKR*>(otcep->RCF);
        if (rc_zkr!=nullptr){
            if (rc_zkr->STATE_ROSPUSK()!=1) continue;
        }
        auto rc=rcs->next_rc[0];
        int recurs_count=0;
        while(rc!=nullptr){
            recurs_count++;
            if (recurs_count>200) break; // защита от бесконечного цикла при сбойном графе

            if (rc->STATE_BUSY()) break;
            if (!rc->rcs->l_otceps.isEmpty()) break;
            auto grc=qobject_cast<m_RC_Gor*>(rc);
            // дальше считаем стрелок нет
            if (grc->MINWAY()==grc->MAXWAY()) break;
            if (grc->MINWAY()>otcep->STATE_MAR()) break;
            if (grc->MAXWAY()<otcep->STATE_MAR()) break;
            auto strel=qobject_cast<m_Strel_Gor_Y*>(rc);
            if (strel){
                if (!l_strel.contains(strel)) break; // избыточно
                // только один отцеп решает
                // но по сути это ситуация когда из 2 точек есть выход на одну рц
                if (strel->rcs->pol_mar!=MVP_Enums::pol_unknow) break;
                auto rcplus=qobject_cast<m_RC_Gor*>(strel->getNextRC(0,0));
                auto rcmnus=qobject_cast<m_RC_Gor*>(strel->getNextRC(0,1));
                if (rcplus==nullptr) break;
                if (rcmnus==nullptr) break;
                bool cmd_plus=false;
                bool cmd_mnus=false;
                if (inway(otcep->STATE_MAR(),rcplus->MINWAY(),rcplus->MAXWAY())) cmd_plus=true;
                if (inway(otcep->STATE_MAR(),rcmnus->MINWAY(),rcmnus->MAXWAY())) cmd_mnus=true;
                if (cmd_plus==cmd_mnus) break;
                if (cmd_plus) strel->rcs->pol_mar=MVP_Enums::pol_plus;
                if (cmd_mnus) strel->rcs->pol_mar=MVP_Enums::pol_minus;
                // до первой необходимости перевода
                if ((strel->STATE_POL()!=strel->rcs->pol_mar)) break;
            }
            // так как только до первой стр с неправ положением - то можем брать просто след
            // а не rcplus/rcmnus
            rc=rc->next_rc[0];
        }
    }

    // определяем возможность команды на перевод
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        if ((TOS->modelGorka->STATE_REGIM()!=ModelGroupGorka::regimRospusk)&&(TOS->modelGorka->STATE_REGIM()!=ModelGroupGorka::regimPausa)) continue;
        if (strel->rcs->pol_mar==MVP_Enums::pol_unknow) continue;
        if (strel->STATE_POL()==strel->rcs->pol_mar) continue;
        if (strel->STATE_A()!=1) continue;
        if (strel->rcs->STATE_CHECK_FREE_DB()==1) continue;
        if (strel->STATE_NEGAB_RC()==1) continue;
        if (strel->STATE_UVK_AV()==1) continue;
        if ((strel->get_rtds()!=nullptr)&&(strel->get_rtds()->STATE_SRAB()==1)) continue;
        if ((strel->get_ipd()!=nullptr)&&(strel->get_ipd()->STATE_SRAB()==1)) continue;
        if (isNegabarit(strel,strel->rcs->pol_mar)) continue;

        strel->rcs->pol_zad=strel->rcs->pol_mar;
    }

    // задаем команды
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        sendCommand(strel,strel->rcs->pol_zad);
    }

    // проверки времени перевода
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        if (strel->rcs->pol_cmd!=MVP_Enums::pol_unknow){
            // определям начало перевода по команде
            if (strel->STATE_POL()==MVP_Enums::pol_w){
                if (!strel->rcs->pol_cmd_w_time.isValid()) strel->rcs->pol_cmd_w_time=T;
            }

            if (strel->rcs->pol_cmd_w_time.isValid()){
                qint64 ms=strel->rcs->pol_cmd_w_time.msecsTo(T);
                if (strel->STATE_POL()!=MVP_Enums::pol_w){
                    // определям автовозврат
                    if (strel->rcs->pol_cmd!=strel->STATE_POL()){
                        if ((ms>100) &&(ms<2000)) {
                            strel->setSTATE_UVK_AV(true);
                        }
                    }
                } else {
                    if ((ms>5000)) {
                        //strel->setSTATE_UVK_CMD_ERROR(true);
                    }
                }
            }
        }
    }
    //сбраcываем таймер
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        if (strel->STATE_POL()!=MVP_Enums::pol_w) strel->rcs->pol_cmd_w_time=QDateTime();
    }



}

void GtGac::sendCommand(m_Strel_Gor_Y *strel, MVP_Enums::TStrelPol pol_cmd,bool force)
{
    if ((strel->rcs->pol_cmd!=pol_cmd)||(force)){
        strel->rcs->pol_cmd=pol_cmd;
        strel->rcs->pol_cmd_time.start();
        if (strel->rcs->pol_cmd==MVP_Enums::pol_plus) {
            emit uvk_command(strel->TU_PRP(),1);
            emit uvk_command(strel->TU_PRM(),0);
            strel->setSTATE_UVK_PRP(true);
            strel->setSTATE_UVK_PRM(false);

        }
        if (strel->rcs->pol_cmd==MVP_Enums::pol_minus) {
            emit uvk_command(strel->TU_PRP(),0);
            emit uvk_command(strel->TU_PRM(),1);
            strel->setSTATE_UVK_PRP(false);
            strel->setSTATE_UVK_PRM(true);
        }
        if (strel->rcs->pol_cmd==MVP_Enums::pol_unknow) {
            emit uvk_command(strel->TU_PRP(),0);
            emit uvk_command(strel->TU_PRM(),0);
            strel->setSTATE_UVK_PRP(false);
            strel->setSTATE_UVK_PRM(false);
        }
    } else {
        // если нет ответа от платы шлем еще
        if ((strel->rcs->pol_cmd_time.isValid()) && (strel->rcs->pol_cmd_time.elapsed()<500)){
            if ((strel->STATE_UVK_PRP()!=strel->STATE_PRP())){
                emit uvk_command(strel->TU_PRP(),strel->STATE_UVK_PRP());
            }
            if ((strel->STATE_UVK_PRM()!=strel->STATE_PRM())){
                emit uvk_command(strel->TU_PRM(),1);
            }
        }
    }
}

void GtGac::state2buffer()
{
    foreach (m_Strel_Gor_Y*strel, l_strel) {
        strel->SIGNAL_UVK_PRP().setValue_1bit(strel->STATE_UVK_PRP());
        strel->SIGNAL_UVK_PRM().setValue_1bit(strel->STATE_UVK_PRM());
        strel->SIGNAL_UVK_AV().setValue_1bit(strel->STATE_UVK_AV());
    }
}

