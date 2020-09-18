// test_main.cpp 测试入口

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QTest>

#define protected public
#define private public
#include "vnoteapplication.h"
#undef protected
#undef private
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
    QApplication app(argc, argv);

    testing::InitGoogleTest();

//    DApplication::loadDXcbPlugin();
//    VNoteApplication app(argc, argv);
//    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
//    app.loadTranslator();
//    app.setOrganizationName("deepin");
//    app.setApplicationName(DEEPIN_VOICE_NOTE);
//    app.setApplicationVersion("1.2.2");
//    app.setProductIcon(QIcon::fromTheme(DEEPIN_VOICE_NOTE));
//    app.setProductName(DApplication::translate("AppMain", "Voice Notes"));
//    app.setApplicationDescription(DApplication::translate("AppMain",
//                                                          "Voice Notes is a lightweight memo tool to make text notes and voice recordings."));

//    qputenv("DTK_USE_SEMAPHORE_SINGLEINSTANCE", "1");

//    if (!DGuiApplicationHelper::instance()->setSingleInstance(
//            app.applicationName(),
//            DGuiApplicationHelper::UserScope)) {
//        return 0;
//    }

//    DApplicationSettings settings;

//    DLogManager::registerConsoleAppender();
//    DLogManager::registerFileAppender();

//    app.activateWindow();
//    qint64 tem = 1;
//    QStringList tmplist;
//    app.onNewProcessInstance(tem, tmplist);

//    VNoteMainWindow* tmpptr = app.mainWindow();

//    int ret = RUN_ALL_TESTS();

//    qInfo() << ret;

//    return app.exec();
    return RUN_ALL_TESTS();
}
