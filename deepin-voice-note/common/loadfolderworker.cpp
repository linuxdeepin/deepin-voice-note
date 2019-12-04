#include "loadfolderworker.h"
#include "vnoteforlder.h"

#include <DLog>

LoadFolderWorker::LoadFolderWorker(QObject *parent)
    : QObject(parent)
    ,QRunnable ()
{

}

void LoadFolderWorker::run()
{
    VNOTE_FOLDERS_MAP* foldersMap = new VNOTE_FOLDERS_MAP();
    VNoteFolder* f1 = new VNoteFolder();
    f1->id =1;
    f1->name = "你好吗";
    foldersMap->insert(f1->id, f1);

    //TODO:
    //    Add load folder code here

    qDebug() << __FUNCTION__ << " load folders ok:" << foldersMap << " thread id:" << QThread::currentThreadId() << "f1:" << f1;

    emit onFoldersLoaded(foldersMap);
}
