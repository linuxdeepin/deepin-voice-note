#ifndef VNOTEAPPLICATION_H
#define VNOTEAPPLICATION_H
#include "views/vnotemainwindow.h"

#include <DApplication>

DWIDGET_USE_NAMESPACE

class VNoteApplication : public DApplication
{
    Q_OBJECT
public:
    explicit VNoteApplication(int &argc, char **argv);

    void activateWindow();
signals:

public slots:
    void onNewProcessInstance(qint64 pid, const QStringList &arguments);
protected:
    virtual void handleQuitAction() override;
protected:
    QScopedPointer<VNoteMainWindow> m_qspMainWnd {nullptr};
};

#endif // VNOTEAPPLICATION_H
