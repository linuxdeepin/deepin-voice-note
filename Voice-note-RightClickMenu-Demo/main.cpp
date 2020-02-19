#include "dnoterightclickmainwindow.h"
#include <DApplication>
#include <DMainWindow>
#include <DWidgetUtil>
#include <QTranslator>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication::loadDXcbPlugin();
    DApplication a(argc, argv);
    //a.loadTranslator();
//    //--
//    QTranslator translator;
//    translator.load("./rcmenu_zh.qm");
//    a.installTranslator(&translator);
//    //--
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    //a.setTheme("light");
    a.setOrganizationName("deepin");
    a.setApplicationName("dtk application");
    a.setApplicationVersion("1.0");
    a.setProductIcon(QIcon(":/images/logo.svg"));
    a.setProductName("Dtk Application");
    a.setApplicationDescription("This is a dtk template application.");
    //sDApplication::translate();
//    DMainWindow w;
//    w.setMinimumSize(800, 600);
//    w.show();
    //--
    QTranslator translator;
    translator.load("./rcmenu_zh.qm");
    qApp->installTranslator(&translator);
    //--
    DNoteRightClickMainWindow w;
    w.setMinimumSize(800, 600);
    w.show();

    Dtk::Widget::moveToCenter(&w);

    return a.exec();
}
