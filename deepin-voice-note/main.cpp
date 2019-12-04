#include "vnoteapplication.h"
#include "views/vnotemainwindow.h"
#include "globaldef.h"

#include <DApplication>
#include <DApplicationSettings>
#include <DGuiApplicationHelper>
#include <DMainWindow>
#include <DLog>
#include <DWidgetUtil>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication::loadDXcbPlugin();
    VNoteApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.loadTranslator();
    app.setOrganizationName("deepin");
    app.setApplicationName(DEEPIN_VOICE_NOTE);
    app.setApplicationVersion(DApplication::buildVersion("1.0"));
    app.setProductIcon(QIcon::fromTheme(DEEPIN_VOICE_NOTE));
    app.setProductName(DApplication::translate("Main", "Voice Note"));
    app.setApplicationDescription(DApplication::translate("Main",
        "Voice Notepad is a lightweight voice tool that provides text notes and voice recordings."));

    qDebug() << "t:" << DApplication::translate("Main", "Voice Note");
    qputenv("DTK_USE_SEMAPHORE_SINGLEINSTANCE", "1");
    if(!DGuiApplicationHelper::instance()->setSingleInstance(
                app.applicationName(),
                DGuiApplicationHelper::UserScope)) {
        return 0;
    }

    DApplicationSettings settings;

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    app.activateWindow();

    return app.exec();
}
