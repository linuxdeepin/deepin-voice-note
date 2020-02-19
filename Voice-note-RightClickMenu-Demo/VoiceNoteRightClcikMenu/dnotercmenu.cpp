#include "dnotercmenu.h"

#include <DApplication>

#include <QSignalMapper>

DNoteRCMenu::DNoteRCMenu(QWidget *parent) : DMenu(parent)
{
    initUi();
}

void DNoteRCMenu::initUi()
{
    QStringList menuTexts;
    menuTexts << DApplication::translate("DNoteRCMenu", "rename")
              << DApplication::translate("DNoteRCMenu", "delete")
              << DApplication::translate("DNoteRCMenu", "newNote");

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

void DNoteRCMenu::slotTriggered(const QString& text)
{
    emit sigTriggered(text);
}

