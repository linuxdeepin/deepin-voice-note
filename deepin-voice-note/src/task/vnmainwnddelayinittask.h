#ifndef VNMAINWNDDELAYINITTASK_H
#define VNMAINWNDDELAYINITTASK_H

#include "vntask.h"
#include "views/vnotemainwindow.h"

#include <QObject>

class VNoteMainWindow;

class VNMainWndDelayInitTask : public VNTask
{
    Q_OBJECT
public:
    explicit VNMainWndDelayInitTask(VNoteMainWindow *pMainWnd
                                    ,QObject *parent = nullptr);

signals:

public slots:

protected:
    virtual void run();
protected:
    VNoteMainWindow* m_pMainWnd {nullptr};
};

#endif // VNMAINWNDDELAYINITTASK_H
