#ifndef LOADFOLDERWORKER_H
#define LOADFOLDERWORKER_H

#include "common/datatypedef.h"
#include "vntask.h"

#include <QObject>
#include <QRunnable>

class LoadFolderWorker : public VNTask
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
