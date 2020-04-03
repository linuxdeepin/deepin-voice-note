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
        //Start audio device watch thread
        //& must be called after initUI
        m_pMainWnd->initAudioWatcher();
        //Init the login manager
        m_pMainWnd->initLogin1Manager();
    }
}
