#ifndef LOADFOLDERWORKER_H
#define LOADFOLDERWORKER_H

#include "datatypedef.h"

#include <QObject>
#include <QRunnable>

class LoadFolderWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit LoadFolderWorker(QObject *parent=nullptr);

protected:
    virtual void run();
signals:
    void onFoldersLoaded(VNOTE_FOLDERS_MAP* foldesMap);
public slots:
};

#endif // LOADFOLDERWORKER_H
