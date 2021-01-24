#ifndef TOS_DSOTRACKING_H
#define TOS_DSOTRACKING_H

#include "tos_dso.h"
#include "tos_rc.h"
#include "baseworker.h"

class tos_System_DSO;

class tos_DsoTracking : public BaseWorker
{
    Q_OBJECT
public:

    explicit tos_DsoTracking(tos_System_DSO *parent,tos_DSO *dso);
    virtual ~tos_DsoTracking(){}
    void resetStates() override;
    void work(const QDateTime &T) override;
    void add2DSO(tos_DSO *dso);

    tos_DSO *tdso;
    tos_DSO *tdso2;
    tos_Rc *rc_next[2];


signals:

public slots:
protected:
    tos_System_DSO *TOS;

};


#endif // TOS_DSOTRACKING_H
