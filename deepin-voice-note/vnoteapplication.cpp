#include "vnoteapplication.h"
#include "globaldef.h"

#include <DWidgetUtil>
#include <DGuiApplicationHelper>

VNoteApplication::VNoteApplication(int &argc, char **argv)
    : DApplication(argc, argv)
{
    connect(DGuiApplicationHelper::instance()
            ,&DGuiApplicationHelper::newProcessInstance, this
            ,&VNoteApplication::onNewProcessInstance);
}

void VNoteApplication::activateWindow()
{
    //Init Normal window at first time
    if (nullptr == m_qspMainWnd.get()) {
        m_qspMainWnd.reset(new VNoteMainWindow());

        m_qspMainWnd->setMinimumSize(DEFAULT_WINDOWS_WIDTH, DEFAULT_WINDOWS_HEIGHT);

        QByteArray mainWindowSize =
                m_qspMainWnd.get()->appSetting()->value(VNOTE_MAINWND_SZ_KEY).toByteArray();

        if (!mainWindowSize.isEmpty()) {
            m_qspMainWnd->restoreGeometry(mainWindowSize);
        }

        //Should be called befor show
        Dtk::Widget::moveToCenter(m_qspMainWnd.get());

        m_qspMainWnd->show();
    } else {
        m_qspMainWnd->setWindowState(Qt::WindowActive);
        m_qspMainWnd->activateWindow();
    }
}

void VNoteApplication::onNewProcessInstance(qint64 pid, const QStringList &arguments)
{
    Q_UNUSED(pid);
    Q_UNUSED(arguments);

    //TODO:
    //Parase comandline here

    activateWindow();
}

void VNoteApplication::handleQuitAction()
{
    //Check if need exit app
    if (m_qspMainWnd->checkIfNeedExit()) {
        DApplication::handleQuitAction();
    }
}
