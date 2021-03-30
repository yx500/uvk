#include "otcepscontroller.h"
#include <QMetaProperty>
#include "mvp_system.h"

OtcepsController::OtcepsController(QObject *parent,m_Otceps *otceps) : BaseWorker(parent)
{
    this->otceps=otceps;
}

void OtcepsController::resetStates()
{
    BaseWorker::resetStates();
    otceps->resetStates();
}

void OtcepsController::work(const QDateTime &)
{
    // выставляем признак текущегог отцепа
    m_Otcep * cur_otcep=nullptr;
    int i_cur_vag=-1;
    //    foreach (auto *otcep, otceps->l_otceps) {
    //        if (!otcep->STATE_ENABLED()) continue;
    //        if (otcep->STATE_ZKR_S_IN()) cur_otcep=otcep;
    //    }
    if (cur_otcep==nullptr){
        foreach (auto *otcep, otceps->l_otceps) {
            if (!otcep->STATE_ENABLED()) continue;
            if (otcep->STATE_ZKR_PROGRESS()==1) {cur_otcep=otcep;break;}
        }
    }
    if (cur_otcep==nullptr){
        foreach (auto *otcep, otceps->l_otceps) {
            if (!otcep->STATE_ENABLED()) continue;
            if (otcep->STATE_LOCATION()==m_Otcep::locationOnPrib) {cur_otcep=otcep;break;}
        }
    }
    // вагон
    if (cur_otcep!=nullptr){
        for (int i=0;i<cur_otcep->vVag.size();i++){
            auto &v=cur_otcep->vVag[i];
            if (v.STATE_ZKR_PROGRESS()==1) i_cur_vag=i;
        }
        if (i_cur_vag<0){
            for (int i=0;i<cur_otcep->vVag.size();i++){
                auto &v=cur_otcep->vVag[i];
                if (v.STATE_LOCATION()==m_Otcep::locationOnPrib) {i_cur_vag=i;break;}
            }
        }
    }
    foreach (auto *otcep, otceps->l_otceps) {
        if (otcep!=cur_otcep) {
            otcep->setSTATE_IS_CURRENT(0);
            for (int i=0;i<otcep->vVag.size();i++){
                auto &v=otcep->vVag[i];
                v.setSTATE_IS_CURRENT(0);
            }
        } else {
            otcep->setSTATE_IS_CURRENT(1);
            if ((i_cur_vag>=0)&&(i_cur_vag<otcep->vVag.size())){
                auto &v=otcep->vVag[i_cur_vag];
                v.setSTATE_IS_CURRENT(0);
            }
        }
    }


}

void OtcepsController::validation(ListObjStr *l) const
{
    BaseWorker::validation(l);
}


QList<SignalDescription> OtcepsController::acceptOutputSignals()
{
    QList<SignalDescription> l;
    // рассыка в обратном порядке
    // 14 сортир 15 от увк

    for (int i=otceps->l_vagons.size()-1;i>=0;i--){
        auto *v=otceps->l_vagons[i];
        v->setSIGNAL_DATA( v->SIGNAL_DATA().innerUse());
        l<<v->SIGNAL_DATA();
    }

    for (int i=otceps->l_otceps.size()-1;i>=0;i--){
        auto *otcep=otceps->l_otceps[i];
        otcep->setSIGNAL_DATA( otcep->SIGNAL_DATA().innerUse());
        l<<otcep->SIGNAL_DATA();
    }

    return l;
}
char __STATE_[]="STATE_";
char __STATE_D_[]="STATE_D_";
void OtcepsController::state2buffer()
{
    foreach (auto *otcep, otceps->l_otceps) {
        {
            t_NewDescr stored_Descr;
            memset(&stored_Descr,0,sizeof(stored_Descr));
            otcep->states2descr_ext(stored_Descr);
            if (otcep->RCS!=nullptr) stored_Descr.D.start  =otcep->RCS->SIGNAL_BUSY_DSO().chanelOffset()+1; else stored_Descr.D.start  =0;
            if (otcep->RCF!=nullptr) stored_Descr.D.end    =otcep->RCF->SIGNAL_BUSY_DSO().chanelOffset()+1; else stored_Descr.D.end  =0;
            otcep->SIGNAL_DATA().setBufferData(&stored_Descr,sizeof(stored_Descr));
        }
    }

    // перестроим модель
    otceps->otceps2Vagons();
    foreach (auto v, otceps->l_vagons) {
        tSlVagon stored_SlVagon;
        memset(&stored_SlVagon,0,sizeof(stored_SlVagon));
        stored_SlVagon=v->toSlVagon();
        v->SIGNAL_DATA().setBufferData(&stored_SlVagon,sizeof(stored_SlVagon));
    }


}

bool OtcepsController::cmd_CLEAR_ALL(QString &acceptStr)
{

    otceps->resetStates();
    foreach (m_Otcep*otcep, otceps->l_otceps) {
        otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
    }
    acceptStr="Сброс списка отцепов.";
    return true;
}

bool OtcepsController::cmd_ACTIVATE_ALL(QString &acceptStr)
{
    foreach (m_Otcep*otcep, otceps->l_otceps) {
        if (otcep->STATE_MAR()>0) otcep->setSTATE_ENABLED(true);
    }
    acceptStr="Активация списка отцепов.";
    return true;
}

bool OtcepsController::cmd_UPDATE_ALL(QString &acceptStr)
{
    foreach (m_Otcep*otcep, otceps->l_otceps) {
        otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
    }
    acceptStr="Обновление списка отцепов.";
    return true;
}

bool OtcepsController::cmd_DEL_OTCEP(QMap<QString, QString> &m, QString &acceptStr)
{
    int N=m["DEL_OTCEP"].toInt();
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        if ((otcep->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)){
            acceptStr=QString("Попытка удалить выявленный отцеп. Отцеп %1.").arg(N);
            return false;
        }
        int n=otceps->l_otceps.indexOf(otcep);
        for (int i=n;i<otceps->l_otceps.size()-1;i++){
            auto otcep_0=otceps->l_otceps[i];
            auto otcep_1=otceps->l_otceps[i+1];
            otcep_0->acceptSLStates(otcep_1);
            otcep_0->setSTATE_EXTNUM(otcep_1->STATE_EXTNUM());
            otcep_0->setSTATE_LOCATION(otcep_1->STATE_LOCATION());
            otcep_0->setSTATE_ENABLED(otcep_1->STATE_ENABLED());
            otcep->inc_tick();

        }
        otceps->l_otceps.last()->resetStates();
        updateVagons();
        acceptStr=QString("Отцеп %1 удален.").arg(N);
        return true;

    }
    acceptStr=QString("Ошибка удаления отцепа %1.").arg(m["N"]);
    return false;
}
m_Otcep* OtcepsController::inc_otcep(int N,int sp)
{
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        int ib=otceps->l_otceps.indexOf(otcep);
        for (int i=otceps->l_otceps.size()-1;i>=ib+1;i--){
            auto otcep_0=otceps->l_otceps[i-1];
            auto otcep_1=otceps->l_otceps[i];
            if ((otcep_0->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) break;
            if ((otcep_1->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) break;
            otcep_1->acceptSLStates(otcep_0);
            otcep_1->setSTATE_EXTNUM(otcep_0->STATE_EXTNUM());
            otcep_1->setSTATE_LOCATION(otcep_0->STATE_LOCATION());
            otcep_1->setSTATE_ENABLED(otcep_0->STATE_ENABLED());
            otcep_1->inc_tick();
        }
        otcep->resetStates();
        otcep->setSTATE_SP(sp);
        otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);

        otcep->inc_tick();
        otcep->setSTATE_ENABLED(true);
        updateVagons();
    }
    return otcep;
}

m_Otcep *OtcepsController::inc_otcep_drobl(int N)
{
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        int cnt_vagon_exit=otcep->STATE_ZKR_VAGON_CNT();

        auto part=otcep->STATE_EXTNUMPART();
        if (part==0){
            part=1;
            otcep->setSTATE_EXTNUMPART(part);
            // проставляем EXTNUM
            int n=otceps->l_otceps.indexOf(otcep);
            if (n>=0){
                for (int i=n;i<otceps->l_otceps.size();i++){
                    auto otcep_1=otceps->l_otceps[i];
                    if (otcep_1->STATE_ENABLED()){
                        if (otcep_1->STATE_EXTNUM()==0) otcep_1->setSTATE_EXTNUM(otcep_1->NUM());
                    }
                }
            }
        }
        auto new_otcep=inc_otcep(N+1,otcep->STATE_SP());
        if (new_otcep!=nullptr){
            int kv=otcep->STATE_SL_VAGON_CNT()-cnt_vagon_exit;
            part++;
            new_otcep->setSTATE_EXTNUMPART(part);
            new_otcep->setSTATE_EXTNUM(otcep->STATE_EXTNUM());
            new_otcep->setSTATE_SL_VAGON_CNT(kv);
            int n=1;
            for (int i=cnt_vagon_exit;i<otcep->vVag.size();i++){
                auto v=otcep->vVag[i];
                v.setSTATE_N_IN_OTCEP(n);n++;
                v.setSTATE_NUM_OTCEP(new_otcep->NUM());
                new_otcep->setVagon(&v);
            }
            otcep->setSTATE_SL_VAGON_CNT_PRED(otcep->STATE_SL_VAGON_CNT());
            otcep->setSTATE_SL_VAGON_CNT(cnt_vagon_exit);

            new_otcep->setSTATE_ENABLED(true);
            new_otcep->resetZKRStates();
            new_otcep->setSTATE_ID_ROSP(otcep->STATE_ID_ROSP());
            new_otcep->setSTATE_SL_VES(otcep->STATE_SL_VES());

            if (otcep->STATE_SL_OSY_CNT()>otcep->STATE_ZKR_OSY_CNT()) new_otcep->setSTATE_SL_OSY_CNT(otcep->STATE_SL_OSY_CNT()-otcep->STATE_ZKR_OSY_CNT());

            updateVagons();
            return new_otcep;
        }
    }
    return nullptr;
}

m_Otcep *OtcepsController::nerascep(int num)
{
    auto exit_otcep=otceps->otcepByNum(num);
    auto  newCurOtcep=exit_otcep;

    // осаживание?
    int sum_all_vag=0;
    for (int i=num-1;i<otceps->l_otceps.size();i++){
        auto otcep=otceps->l_otceps[i];
        if (otcep->STATE_ENABLED()) sum_all_vag+=otcep->STATE_SL_VAGON_CNT();

    }
    if (exit_otcep->STATE_ZKR_VAGON_CNT()>=sum_all_vag) return nullptr;


    int cnt_vagon_perebor=exit_otcep->STATE_ZKR_VAGON_CNT()-exit_otcep->STATE_SL_VAGON_CNT();

    for (int i=num;i<otceps->l_otceps.size();i++){
        auto next_otcep=otceps->l_otceps[i];
        if ((next_otcep->STATE_ENABLED()==0)||(next_otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)) return newCurOtcep;
        if ((next_otcep->STATE_SL_VAGON_CNT()==0)) return newCurOtcep;
        int cnt_vagon_delete=cnt_vagon_perebor;
        if (cnt_vagon_delete>next_otcep->STATE_SL_VAGON_CNT()) cnt_vagon_delete=next_otcep->STATE_SL_VAGON_CNT();

        // добавляем к ушедшему
        int n=exit_otcep->vVag.size()+1;
        exit_otcep->setSTATE_SL_VAGON_CNT(exit_otcep->vVag.size()+cnt_vagon_delete);

        for (int iv=0;iv<cnt_vagon_delete;iv++){
            if (iv<next_otcep->vVag.size()){
                auto v=next_otcep->vVag[iv];
                v.setSTATE_N_IN_OTCEP(n);
                n++;
                v.setSTATE_NUM_OTCEP(exit_otcep->NUM());
                exit_otcep->setVagon(&v);
            }
        }
        // удаляем оцеп
        if (cnt_vagon_delete>=next_otcep->vVag.size()){
            next_otcep->setSTATE_SL_VAGON_CNT_PRED(next_otcep->STATE_SL_VAGON_CNT());
            next_otcep->setSTATE_SL_VAGON_CNT(0);
            next_otcep->resetTrackingStates();
            next_otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
            if (i+1<otceps->l_otceps.size()) {
                newCurOtcep=otceps->l_otceps[i+1];
            }
        } else {
            // удаляем удаленные
            n=1;
            for (int i=0;i<next_otcep->vVag.size()-cnt_vagon_delete-1;i++){
                auto &v0=next_otcep->vVag[i];
                auto &v1=next_otcep->vVag[i+1];
                v0.assign(&v1);
                v0.setSTATE_N_IN_OTCEP(n);n++;
            }
            next_otcep->setSTATE_SL_VAGON_CNT_PRED(next_otcep->STATE_SL_VAGON_CNT());
            next_otcep->setSTATE_SL_VAGON_CNT(next_otcep->vVag.size()-cnt_vagon_delete);
            break;
        }
        cnt_vagon_perebor=cnt_vagon_perebor-cnt_vagon_delete;
        if (cnt_vagon_perebor<=0) break;
    }//  for (int i=num;i<otcepsController->otceps->l_otceps.size();i++){
    return newCurOtcep;
}

bool OtcepsController::cmd_INC_OTCEP(QMap<QString, QString> &m, QString &acceptStr)
{
    int N=m["INC_OTCEP"].toInt();
    int SP=m["SP"].toInt();
    auto otcep=otceps->otcepByNum(N);
    if (otcep!=nullptr){
        if ((otcep->STATE_ENABLED()) && (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)){
            acceptStr=QString("Попытка добавить отцеп перед выявленным. Отцеп %1.").arg(N);
            return false;
        }
        if (otcep->NUM()>1){
            auto otcep1=otceps->otcepByNum(N-1);
            if (otcep1!=nullptr){
                if (otcep1->STATE_LOCATION()!=m_Otcep::locationOnPrib){
                    acceptStr=QString("Попытка добавить отцеп вне очереди. Отцеп %1.").arg(N);
                    return false;
                }
            }
        }
        inc_otcep(N,SP);

        acceptStr=QString("Отцеп %1 добавлен.").arg(N);
        updateVagons();
        if (otcep->NUM()==1){
            auto ID_ROSP=otcep->STATE_ID_ROSP();
            if (ID_ROSP==0){
                time_t n;
                time(&n);
                uint32 r = n * 16;
                ID_ROSP=r;
                setNewID_ROSP(ID_ROSP);
            }
        }
        return true;
    }
    acceptStr=QString("Ошибка добавления отцепа %1.").arg(m["N"]);
    return false;
}

bool OtcepsController::cmd_SET_CUR_OTCEP(QMap<QString, QString> &m, QString &acceptStr)
{
    int N=m["SET_CUR_OTCEP"].toInt();
    auto otcep1=otceps->otcepByNum(N);
    if ((otcep1!=nullptr)&&(otcep1->STATE_ENABLED())) {
        // все белые до санут серыми
        bool befor=true;
        foreach (auto otcep, otceps->l_otceps) {
            if (otcep==otcep1) {
                befor=false;
            }
            if (befor){
                if ((otcep->STATE_ENABLED())&&(otcep->STATE_LOCATION()==m_Otcep::locationOnPrib)){
                    otcep->setSTATE_LOCATION(m_Otcep::locationUnknow);
                    otcep->setSTATE_GAC_ACTIVE(0);
                    otcep->setSTATE_ERROR(0);
                    otcep->resetTrackingStates();
                }
            } else {
                if (otcep->STATE_ENABLED()) {
                    otcep->resetTrackingStates();
                    otcep->setSTATE_LOCATION(m_Otcep::locationOnPrib);
                    otcep->setSTATE_GAC_ACTIVE(0);
                    otcep->setSTATE_MAR_F(0);
                    otcep->setSTATE_ERROR(0);

                }

            }
        }
        acceptStr=QString("Отцеп %1 стал текущим.").arg(N);
        updateVagons();
        return true;
    }

    acceptStr=QString("Ошибка установки текущего отцепа %1.").arg(m["N"]);
    return false;
}

bool OtcepsController::cmd_CHECK_LIST(QMap<QString, QString> &m, QString &acceptStr)
{
    quint32 ID_ROSP=m["ID_ROSP"].toInt();
    int OTCEP_CNT=m["OTCEP_CNT"].toInt();
    int VAGON_CNT=m["VAGON_CNT"].toInt();

    int _OTCEP_CNT=0;
    int _VAGON_CNT=0;
    foreach (m_Otcep*otcep, otceps->l_otceps) {
        if (otcep->STATE_ENABLED()){
            _OTCEP_CNT++;
            _VAGON_CNT+=otcep->vVag.size();
        }
    }
    if ((ID_ROSP!=otceps->l_otceps.first()->STATE_ID_ROSP())||
            (OTCEP_CNT!=_OTCEP_CNT)||
            (VAGON_CNT!=_VAGON_CNT)
            )
    {
        otceps->resetStates();
        foreach (m_Otcep*otcep, otceps->l_otceps) {
            otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
        }
        acceptStr="Сбой загрузки СЛ.";
        return false;
    }
    acceptStr="Сортировочный листок загружен.";
    return true;
}

bool OtcepsController::cmd_SET_OTCEP_STATE(QMap<QString, QString> &m, QString &acceptStr)
{
    if (!m["N"].isEmpty()){

        int N=m["N"].toInt();
        auto otcep=otceps->otcepByNum(N);
        if (otcep!=nullptr){
            if (((otcep->STATE_LOCATION()==m_Otcep::locationOnSpusk)||(otcep->STATE_LOCATION()==m_Otcep::locationOnPark))
                    &&(!otcep->STATE_IS_CURRENT())) {
                acceptStr=QString("Отцеп %1 попытка изменить выявленный отцеп.").arg(N);
                return false;
            }
            bool ex_change=false;
            foreach (QString key, m.keys()) {
                if ((key=="CMD") || (key=="N")) continue;
                QString stateName="STATE_"+key;
                QVariant V;
                V=m[key];

                for (int idx = 0; idx < otcep->metaObject()->propertyCount(); idx++) {
                    QMetaProperty metaProperty = otcep->metaObject()->property(idx);
                    if (metaProperty.name()!=stateName) continue;
                    QVariant V1=otcep->property(qPrintable(stateName));
                    if (V1!=V){
                        qDebug()<< stateName << V1.toString() << V.toString();
                        ex_change=true;
                        //otcep->setProperty(qPrintable(stateName),V);
                        metaProperty.write(otcep,V);
                    }

                }
            }
            if (otcep->NUM()==1){
                if (m.keys().contains("ENABLED")){
                    if (m["ENABLED"]=="1"){
                        auto ID_ROSP=otcep->STATE_ID_ROSP();
                        if (ID_ROSP==0){
                            time_t n;
                            time(&n);
                            uint32 r = n * 16;
                            ID_ROSP=r;
                            setNewID_ROSP(ID_ROSP);
                        }
                    }
                }
            }

            if (ex_change){
                acceptStr=QString("Отцеп %1 свойства изменены.").arg(N);
                otcep->inc_tick();
            }
            //            updateVagons();
            return true;
        }
    }
    acceptStr=QString("Ошибка изменения свойств отцепа %1.").arg(m["N"]);
    return false;
}

bool OtcepsController::cmd_SET_VAGON_STATE(QMap<QString, QString> &m, QString &acceptStr)
{
    if (!m["N"].isEmpty()){

        int N=m["N"].toInt();
        auto otcep1=otceps->otcepByNum(N);
        if (otcep1!=nullptr){
            int N_IN_OTCEP=m["N_IN_OTCEP"].toInt();
            if ((N_IN_OTCEP<=0)||(N_IN_OTCEP>otcep1->vVag.size())){
                acceptStr=QString("Ошибка: Номер в отцепе %1 больше кол-ва по СЛ. %2 .").arg(N_IN_OTCEP).arg(otcep1->vVag.size());
                return false;
            }
            auto vagon=&otcep1->vVag[N_IN_OTCEP-1];
            bool ex_change=false;
            foreach (QString key, m.keys()) {
                if ((key=="CMD") || (key=="N")|| (key=="N_IN_OTCEP")) continue;
                QString stateName="STATE_"+key;
                QVariant V;
                V=m[key];

                for (int idx = 0; idx < vagon->metaObject()->propertyCount(); idx++) {
                    QMetaProperty metaProperty = vagon->metaObject()->property(idx);
                    if (metaProperty.name()!=stateName) continue;
                    QVariant V1=vagon->property(qPrintable(stateName));
                    if (V1!=V){
                        qDebug()<< stateName << V1.toString() << V.toString();
                        ex_change=true;
                        metaProperty.write(vagon,V);
                    }

                }
            }

            if (ex_change){
                acceptStr=QString("Отцеп %1 вагон %2 свойства изменены.").arg(N).arg(N_IN_OTCEP);
                otcep1->inc_tick();
            }
            return true;
        }
    }
    acceptStr=QString("Ошибка изменения свойств вагона в отцепе %1").arg(m["N"]);
    return false;
}

//bool OtcepsController::cmd_ADD_OTCEP_VAG(QMap<QString, QString> &m, QString &acceptStr)
//{
//    if (!m["N"].isEmpty()){
//        int N=m["N"].toInt();
//        auto otcep=otceps->otcepByNum(N);
//        if (otcep!=nullptr){



//            QVariantHash mv;
//            foreach (QString key, m.keys()) {
//                mv[key]=m[key];
//            }
//            tSlVagon v=Map2tSlVagon(mv);
//            if (v.Id!=otcep->STATE_ID_ROSP()) {
//                acceptStr=QString("Отцеп %1 ваг. %2 v.Id!=otcep->STATE_ID_ROSP().").arg(m["N"]).arg(m["IV"]);
//                return false;
//            }
//            if (v.NO!=otcep->NUM()) {
//                acceptStr=QString("Отцеп %1 ваг. %2 v.NO!=otcep->NUM().").arg(m["N"]).arg(m["IV"]);
//                return false;
//            };
//            if ((v.IV<=0)||(v.IV>MaxVagon)) {
//                acceptStr=QString("Отцеп %1 ваг. %2 (v.IV<=0)||(v.IV>MaxVagon).").arg(m["N"]).arg(m["IV"]);
//                return false;
//            };



//            int NV=m["NV"].toInt();
//            if (NV>otcep->vVag.size()){
//                acceptStr=QString("Ошибка добавления: Номер в отцепе %1 больше кол-ва по СЛ. %2 .").arg(NV).arg(otcep->vVag.size());
//                return false;

//                //                int add_cnt=NV-otcep->vVag.size();
//                //                for (int i=0;i<add_cnt;i++){
//                //                    otcep->vVag.push_back(tSlVagon());
//                //                }
//            }
//            otcep->vVag[NV-1].fromSlVagon(v);
//            //otceps->vagons[v.IV-1]=v;
//            acceptStr=QString("Отцеп %1 добавлен ваг. %2 .").arg(m["N"]).arg(m["IV"]);
//            updateVagons();
//            return true;
//        }
//    }
//    acceptStr=QString("Ошибка добавления: Отцеп %1 добавлен ваг. %2 .").arg(m["N"]).arg(m["IV"]);
//    return false;
//}

void OtcepsController::resetLiveOtceps()
{
    foreach (auto otcep, otceps->otceps()) {
        if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)
            otcep->resetStates();
        //otcep->setSTATE_TICK(otcep->STATE_TICK()+1);
    }

}

void OtcepsController::finishLiveOtceps()
{
    foreach (auto otcep, otceps->otceps()) {
        if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib)
            otcep->setSTATE_LOCATION(m_Otcep::locationUnknow); // нужно для старых
    }
}

void OtcepsController::setNewID_ROSP(quint32 ID_ROSP)
{
    foreach (auto otcep, otceps->otceps()) {
        if (otcep->STATE_LOCATION()!=m_Otcep::locationOnPrib){
            otcep->setSTATE_ID_ROSP(ID_ROSP);
            otcep->inc_tick();
        }
    }
}

void OtcepsController::updateVagons()
{
    otceps->l_otceps.first()->inc_tick();
}

