#include "dnoterightclickmainwindow.h"
#include "VoiceNoteRightClcikMenu/dnotercmenu.h"
#include "VoiceNoteRightClcikMenu/ddirmenu.h"
#include "VoiceNoteRightClcikMenu/dvoicemenu.h"

#include <QMouseEvent>
#include <QMenuBar>
#include <QMessageBox>

DNoteRightClickMainWindow::DNoteRightClickMainWindow(DMainWindow *parent) : DMainWindow(parent)
{

}

void DNoteRightClickMainWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        DNoteRCMenu noteMenu(this);
        connect(&noteMenu, SIGNAL(sigTriggered(QString)),
                this, SLOT(OnToggledNoteMenu(QString)));

        QPoint cursorPos = QCursor::pos();
        //cursorPos.setX(cursorPos.x() - 50);
        noteMenu.exec(cursorPos);

        DDirMenu dirMenu(this);
        connect(&dirMenu, SIGNAL(sigTriggered(QString)),
                this, SLOT(OnToggledNoteMenu(QString)));
        dirMenu.exec(QCursor::pos());

        DVoiceMenu voiceMenu(this);
        connect(&voiceMenu, SIGNAL(sigTriggered(QString)),
                this, SLOT(OnToggledNoteMenu(QString)));
        QPoint cursorPos1 = QCursor::pos();
        //cursorPos.setX(cursorPos.x() + 50);
        voiceMenu.exec(cursorPos1);
    }
}

void DNoteRightClickMainWindow::OnToggledNoteMenu(const QString& text)
{
    QMessageBox::information(this, "            ", text);
}

