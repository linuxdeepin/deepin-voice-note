#include "vnmainwnddelayinittask.h"
#include "common/vnotedatasafefymanager.h"
#include "views/vnotemainwindow.h"

VNMainWndDelayInitTask::VNMainWndDelayInitTask(VNoteMainWindow *pMainWnd, QObject *parent)
    : VNTask(parent)
    , m_pMainWnd(pMainWnd)
{

}

void VNMainWndDelayInitTask::run()
{
    VNoteDataSafefyManager::instance()->reqSafers();

    if (nullptr != m_pMainWnd) {
        //Delay initialize work
        m_pMainWnd->initDelayWork();
    }
}
