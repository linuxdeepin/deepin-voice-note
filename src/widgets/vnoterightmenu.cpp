#include "vnoterightmenu.h"

VNoteRightMenu::VNoteRightMenu(QWidget *parent)
    : DMenu(parent)
    , m_timer(new QTimer(this))
{
    initConnections();
}

VNoteRightMenu::~VNoteRightMenu()
{
}

void VNoteRightMenu::initConnections()
{
    connect(m_timer, &QTimer::timeout, this, [ = ] {
        if (qAbs(QCursor::pos().y() - m_pressPointY) > 8)
            m_moved = true;
        else
        {
            m_moved = false;
        }

    });
    connect(this, &VNoteRightMenu::aboutToShow, this, [ = ] {
        m_timer->start(50);
    });
    connect(this, &VNoteRightMenu::aboutToHide, this, [ = ] {
        m_timer->stop();
    });
}

void VNoteRightMenu::mouseMoveEvent(QMouseEvent *eve)
{
    if (eve->source() == Qt::MouseEventSynthesizedByQt) {
        if (m_moved) {
            m_timer->stop();
            emit menuTouchMoved();
        }
    }
    DMenu::mouseMoveEvent(eve);
}

void VNoteRightMenu::mouseReleaseEvent(QMouseEvent *eve)
{
    if (eve->source() == Qt::MouseEventSynthesizedByQt) {
        emit menuTouchReleased();
        m_timer->stop();

    }
    return DMenu::mouseReleaseEvent(eve);
}

void VNoteRightMenu::closeEvent(QCloseEvent *eve)
{
    DMenu::closeEvent(eve);
}

void VNoteRightMenu::setPressPointY(int value)
{
    m_pressPointY = value;
}


