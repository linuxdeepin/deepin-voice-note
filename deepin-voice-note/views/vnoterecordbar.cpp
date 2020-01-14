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
    m_recordPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_recordPanel->setVisible(false);

    m_recordBtn = new VNoteIconButton(this
                                      , "audio_normal.svg"
                                      , "audio_hover.svg"
                                      , "audio_press.svg");
    m_recordBtn->setFlat(true);
    m_recordBtn->setIconSize(QSize(REC_BTN_W, REC_BTN_H));
    //m_recordBtn->setFixedSize(QSize(55, 55));

    m_recBtnAnchor.reset(new DAnchorsBase(m_recordBtn));
    m_recBtnAnchor->setAnchor(Qt::AnchorLeft, this, Qt::AnchorLeft);
    m_recBtnAnchor->setAnchor(Qt::AnchorBottom, this, Qt::AnchorBottom);

    rightNoteAreaLayout->addWidget(m_recordPanel);
}

void VNoteRecordBar::initConnections()
{
    connect(m_recordBtn, &VNoteIconButton::clicked, this, &VNoteRecordBar::onStartRecord);
    connect(m_recordPanel, SIGNAL(sigFinshRecord(const QString &,qint64)),
            this, SLOT(onFinshRecord(const QString &,qint64)));
}

void VNoteRecordBar::resizeEvent(QResizeEvent *event)
{
    int leftMargin   = (event->size().width() - REC_BTN_W) / 2;
    int bottonMargin = (event->size().height() - REC_BTN_H) / 2;

    m_recBtnAnchor->setBottomMargin(bottonMargin);
    m_recBtnAnchor->setLeftMargin(leftMargin);
}

void VNoteRecordBar::onStartRecord()
{
    m_recordPanel->setVisible(true);
    m_recordBtn->setVisible(false);
    m_recordPanel->startRecord();
    emit sigStartRecord();
}

void VNoteRecordBar::onFinshRecord(const QString &voicePath,qint64 voiceSize)
{
    m_recordPanel->setVisible(false);
    m_recordBtn->setVisible(true);
    emit sigFinshRecord(voicePath,voiceSize);
}
