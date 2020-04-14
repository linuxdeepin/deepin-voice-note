#include "loadfolderworker.h"
#include "common/vnoteforlder.h"
#include "db/vnotefolderoper.h"
#include "globaldef.h"

#include <DLog>

LoadFolderWorker::LoadFolderWorker(QObject *parent)
    : VNTask(parent)
{

}

void LoadFolderWorker::run()
{
    static struct timeval start,backups, end;

    gettimeofday(&start, nullptr);
    backups = start;

    VNoteFolderOper folderOper;
    VNOTE_FOLDERS_MAP* foldersMap = folderOper.loadVNoteFolders();

    gettimeofday(&end, nullptr);

    qDebug() << "LoadFolderWorker(ms):" << TM(start, end);

    //TODO:
    //    Add load folder code here

    qDebug() << __FUNCTION__ << " load folders ok:" << foldersMap << " thread id:" << QThread::currentThreadId();

    emit onFoldersLoaded(foldersMap);
}
