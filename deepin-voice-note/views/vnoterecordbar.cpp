#include "vnoterecordbar.h"
#include "widgets/vnoterecordwidget.h"
#include "widgets/vnoteiconbutton.h"

#include <QVBoxLayout>

#include <DLog>

VNoteRecordBar::VNoteRecordBar(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    initConnections();
}

void VNoteRecordBar::initUI()
{
    QVBoxLayout *rightNoteAreaLayout = new QVBoxLayout(this);
    rightNoteAreaLayout->setSpacing(0);
    rightNoteAreaLayout->setContentsMargins(0, 0, 0, 0);

    m_recordPanel = new VNoteRecordWidget(this);
    m_recordPanel->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    m_recordPanel->setVisible(false);

    m_recordBtn = new VNoteIconButton(":/images/icon/normal/circlebutton_voice.svg"
                                      ,":/images/icon/normal/circlebutton_voice.svg"
                                      ,":/images/icon/normal/circlebutton_voice.svg"
                                      ,this);
    m_recordBtn->setFlat(true);
    m_recordBtn->setIconSize(QSize(68, 68));
    //m_recordBtn->setFixedSize(QSize(55, 55));

    m_recBtnAnchor.reset(new DAnchorsBase(m_recordBtn));
    m_recBtnAnchor->setAnchor(Qt::AnchorLeft, this, Qt::AnchorLeft);
    m_recBtnAnchor->setAnchor(Qt::AnchorBottom, this, Qt::AnchorBottom);

    qDebug() << "rect:" << this->rect();

    rightNoteAreaLayout->addWidget(m_recordPanel);
}

void VNoteRecordBar::initConnections()
{

}

void VNoteRecordBar::resizeEvent(QResizeEvent *event)
{
    int leftMargin   = (event->size().width()-REC_BTN_W)/2;
    int bottonMargin = (event->size().height()-REC_BTN_H)/2;

    m_recBtnAnchor->setBottomMargin(bottonMargin);
    m_recBtnAnchor->setLeftMargin(leftMargin);

    qDebug() << "rect:" << leftMargin << bottonMargin;
}
