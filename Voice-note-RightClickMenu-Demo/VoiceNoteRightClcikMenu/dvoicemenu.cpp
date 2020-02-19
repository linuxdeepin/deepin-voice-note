#include "dvoicemenu.h"

#include <DApplication>

#include <QSignalMapper>

DVoiceMenu::DVoiceMenu(QWidget *parent) : DMenu(parent)
{
    initUi();
}
void DVoiceMenu::initUi()
{
    QStringList menuTexts;
    menuTexts << DApplication::translate("DVoiceMenu", "delete")
              << DApplication::translate("DVoiceMenu", "save as mp3")
              << DApplication::translate("DVoiceMenu", "voice turn words");

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

void DVoiceMenu::slotTriggered(const QString& text)
{
    emit sigTriggered(text);
}

