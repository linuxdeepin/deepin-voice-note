#include "loadsafeteydataworker.h"
#include "db/vnotesaferoper.h"
#include "globaldef.h"

#include <DLog>

LoadSafeteyDataWorker::LoadSafeteyDataWorker(QObject *parent)
    : VNTask(parent)
{

}

void LoadSafeteyDataWorker::run()
{
    struct timeval start, end;
    gettimeofday(&start, nullptr);

    //TODO:
    //    Add load folder code here
    VNoteSaferOper saferOper;
    SafetyDatas *safers = saferOper.loadSafers();


    gettimeofday(&end, nullptr);

    qDebug() << "Load safers ok:" << safers->size()
             << " cost time(ms):" << TM(start, end)
             << " thread id:" << QThread::currentThreadId();

    emit saferLoaded(safers);
}
