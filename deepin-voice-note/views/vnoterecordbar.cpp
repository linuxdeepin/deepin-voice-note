#include "vnoterecordbar.h"
#include "widgets/vnoterecordwidget.h"
#include "widgets/vnoteiconbutton.h"
#include "widgets/vnoteplaywidget.h"

#include "common/vnoteitem.h"

#include <QVBoxLayout>

#include <DApplication>
#include <DLog>

VNoteRecordBar::VNoteRecordBar(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    initConnections();
}

void VNoteRecordBar::initUI()
{
    m_mainLayout = new QStackedLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_recordPanel = new VNoteRecordWidget(this);
    m_recordPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_mainLayout->addWidget(m_recordPanel);

    m_recordBtnHover = new DWidget(this);
    m_recordBtn = new VNoteIconButton(m_recordBtnHover
                                      , "audio_normal.svg"
                                      , "audio_hover.svg"
                                      , "audio_press.svg");
    m_recordBtn->SetDisableIcon("audio_disabled.svg");
    m_recordBtn->setFlat(true);
    m_recordBtn->setIconSize(QSize(REC_BTN_W, REC_BTN_H));
    m_recordBtn->setFixedSize(QSize(REC_BTN_W, REC_BTN_H));
    QGridLayout *recordBtnHoverLayout = new QGridLayout;
    recordBtnHoverLayout->addWidget(m_recordBtn,0,1);
    recordBtnHoverLayout->setColumnStretch(0,1);
    recordBtnHoverLayout->setColumnStretch(1,0);
    recordBtnHoverLayout->setColumnStretch(2,1);
    m_recordBtnHover->setLayout(recordBtnHoverLayout);
    m_mainLayout->addWidget(m_recordBtnHover);

    m_playPanel = new VNotePlayWidget(this);
    m_mainLayout->addWidget(m_playPanel);
    //Default unavailable
    OnMicrophoneAvailableChanged(false);
    m_mainLayout->setCurrentWidget(m_recordBtnHover);
}

void VNoteRecordBar::initConnections()
{
    //Install filter to filte MousePress
    //event.
    installEventFilter(this);
    m_recordBtn->installEventFilter(this);

    connect(m_recordBtn, &VNoteIconButton::clicked, this, &VNoteRecordBar::onStartRecord);
    connect(m_recordPanel, SIGNAL(sigFinshRecord(const QString &,qint64)),
            this, SLOT(onFinshRecord(const QString &,qint64)));
    connect(m_playPanel, &VNotePlayWidget::sigWidgetClose,
            this, &VNoteRecordBar::onClosePlayWidget);
    connect(m_playPanel, SIGNAL(sigPlayVoice(VNVoiceBlock *)),
            this, SIGNAL(sigPlayVoice(VNVoiceBlock *)));
    connect(m_playPanel, SIGNAL(sigPauseVoice(VNVoiceBlock *)),
            this, SIGNAL(sigPauseVoice(VNVoiceBlock *)));

}

bool VNoteRecordBar::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);
    //Let NoteItem lost focus when click
    //outside of Note

    if (e->type() == QEvent::MouseButtonPress) {
        setFocus(Qt::MouseFocusReason);
    }

    return false;
}

void VNoteRecordBar::onStartRecord()
{
    m_mainLayout->setCurrentWidget(m_recordPanel);
    m_recordPanel->startRecord();
    emit sigStartRecord();
}

void VNoteRecordBar::onFinshRecord(const QString &voicePath,qint64 voiceSize)
{
    m_mainLayout->setCurrentWidget(m_recordBtnHover);
    emit sigFinshRecord(voicePath,voiceSize);
}

void VNoteRecordBar::OnMicrophoneAvailableChanged(int availableState)
{
    qInfo() << __FUNCTION__ << " MicrophoneState:" << availableState;
//    if (isAvailable) {
//        m_recordBtn->setBtnDisabled(false);
//        m_recordBtn->setToolTip("");
//    } else {
//        m_recordBtn->setBtnDisabled(true);
//        m_recordBtn->setToolTip(
//                    DApplication::translate(
//                        "VNoteRecordBar",
//                        "No recording device detected")
//                    );
//    }
}

void VNoteRecordBar::cancelRecord()
{
    m_recordPanel->cancelRecord();
}

void VNoteRecordBar::onClosePlayWidget(VNVoiceBlock *voiceData)
{
    m_mainLayout->setCurrentWidget(m_recordBtnHover);
    emit sigWidgetClose(voiceData);
}

void VNoteRecordBar::playVoice(VNVoiceBlock *voiceData)
{
    m_mainLayout->setCurrentWidget(m_playPanel);
    m_playPanel->setVoiceBlock(voiceData);
}

void VNoteRecordBar::pauseVoice(VNVoiceBlock *voiceData)
{
    if(m_mainLayout->currentWidget() == m_playPanel
            && m_playPanel->getVoiceData() == voiceData){
        m_playPanel->onPauseBtnClicked();
    }
}
bool VNoteRecordBar::stopVoice(VNVoiceBlock *voiceData)
{
    if(m_mainLayout->currentWidget() == m_playPanel
            && m_playPanel->getVoiceData() == voiceData){
        m_playPanel->stopVideo();
        m_mainLayout->setCurrentWidget(m_recordBtnHover);
        return true;
    }
    return false;
}

 VNVoiceBlock* VNoteRecordBar::getVoiceData()
 {
     if(m_mainLayout->currentWidget() == m_playPanel){
         return  m_playPanel->getVoiceData();
     }
     return nullptr;
 }
