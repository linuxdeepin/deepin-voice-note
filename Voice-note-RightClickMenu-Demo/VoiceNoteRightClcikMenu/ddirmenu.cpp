#include "ddirmenu.h"

#include <DApplication>

#include <QSignalMapper>

DDirMenu::DDirMenu(QWidget *parent) : DMenu(parent)
{
    initUi();
}
void DDirMenu::initUi()
{
    QStringList menuTexts;
    menuTexts << DApplication::translate("DDirMenu", "rename")
              << DApplication::translate("DDirMenu", "delete")
              << DApplication::translate("DDirMenu", "save as TXT")
              << DApplication::translate("DDirMenu", "save attachment")
              << DApplication::translate("DDirMenu", "new Note");

    QSignalMapper *pMapper = new QSignalMapper(this);
    for(int i = 0; i < menuTexts.size(); i++)
    {
        QAction* pAction = new QAction(menuTexts[i], this);
        addAction(pAction);
        connect(pAction, SIGNAL(triggered()), pMapper, SLOT(map()));
        pMapper->setMapping(pAction, pAction->text());
    }

    connect(pMapper, SIGNAL(mapped(const QString&)),
            this, SLOT(slotTriggered(const QString&)));
}

void DDirMenu::slotTriggered(const QString& text)
{
    emit sigTriggered(text);
}

