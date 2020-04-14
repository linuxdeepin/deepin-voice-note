#ifndef VNOTEAPPLICATION_H
#define VNOTEAPPLICATION_H
#include "views/vnotemainwindow.h"

#include <QSettings>

#include <DApplication>

DWIDGET_USE_NAMESPACE

class VNoteApplication : public DApplication
{
    Q_OBJECT
public:
    explicit VNoteApplication(int &argc, char **argv);

    void activateWindow();
    /*
     * Comment:
     *     In order put our config file to deepin's config
     * dir,we should call the this function in main window.
     * We can't get correct path when app's consturctor.
     * */
    void initAppSetting();
    QSharedPointer<QSettings> appSetting() const;
    VNoteMainWindow* mainWindow() const;
signals:

public slots:
    void onNewProcessInstance(qint64 pid, const QStringList &arguments);
protected:
    virtual void handleQuitAction() override;
protected:
    QScopedPointer<VNoteMainWindow> m_qspMainWnd {nullptr};

    //App setting
    QSharedPointer<QSettings> m_qspSetting {nullptr};
};

#endif // VNOTEAPPLICATION_H
