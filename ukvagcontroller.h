#ifndef UKVAGCONTROLLER_H
#define UKVAGCONTROLLER_H

#include "baseworker.h"
#include "m_ukvag.h"

class ModelGroupGorka;
class m_Otceps;

class UkVagController : public BaseWorker
{
    Q_OBJECT
public:
    explicit UkVagController(QObject *parent,ModelGroupGorka *GORKA);
    virtual ~UkVagController()override{}
    void work(const QDateTime &T) override;
    QList<SignalDescription> acceptOutputSignals() override;
    void state2buffer()override;

    ModelGroupGorka *GORKA;
    m_Otceps *otceps;
    m_UkVag * ukvag;

signals:

};

#endif // UKVAGCONTROLLER_H
