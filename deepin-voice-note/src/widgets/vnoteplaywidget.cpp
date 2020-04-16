#include "vnoteplaywidget.h"
#include "vnote2siconbutton.h"

#include "common/vnoteitem.h"
#include "common/utils.h"

#include <QGridLayout>

#include <DDialogCloseButton>

VNotePlayWidget::VNotePlayWidget(QWidget *parent)
    : DFloatingWidget(parent)
{
    initUI();
    initPlayer();
    initConnection();
}

void VNotePlayWidget::initUI()
{
    m_playerBtn = new VNote2SIconButton("play.svg", "pause.svg", this);
    m_playerBtn->setIconSize(QSize(40, 40));
    m_playerBtn->setFixedSize(QSize(48, 48));
    m_playerBtn->setFocusPolicy(Qt::NoFocus);

    m_timeLab = new DLabel(this);
    m_timeLab->setText("00:00/00:00");
    m_timeLab->setFixedWidth(124);
    m_sliderHover = new DWidget(this);
    m_nameLab = new DLabel(m_sliderHover);
    m_nameLab->setContentsMargins(8,0,0,0);
    m_slider = new DSlider(Qt::Horizontal, m_sliderHover);
    m_slider->setMinimum(0);
    m_slider->setValue(0);

    m_closeBtn = new DDialogCloseButton(this);
    m_closeBtn->setIconSize(QSize(26,26));
    DStyle::setFocusRectVisible(m_closeBtn, false);

    QVBoxLayout *sliderLayout = new QVBoxLayout;
    sliderLayout->addWidget(m_nameLab, Qt::AlignLeft);
    sliderLayout->addWidget(m_slider);
    m_sliderHover->setLayout(sliderLayout);
    sliderLayout->setContentsMargins(0, 5, 0, 5);
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_playerBtn);
    mainLayout->addWidget(m_sliderHover);
    mainLayout->addWidget(m_timeLab, 0, Qt::AlignLeft);
    mainLayout->addWidget(m_closeBtn);
    mainLayout->setContentsMargins(13, 0, 13, 0);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    this->setLayout(mainLayout);
    m_sliderHover->installEventFilter(this);
    //m_slider->installEventFilter(this);
}

void VNotePlayWidget::initPlayer()
{
    m_player = new QMediaPlayer(this);
}

void VNotePlayWidget::initConnection()
{
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &VNotePlayWidget::onVoicePlayPosChange);
    connect(m_player,&QMediaPlayer::durationChanged,
            this,&VNotePlayWidget::onDurationChanged);

    connect(m_playerBtn, &VNote2SIconButton::clicked,
             this, &VNotePlayWidget::onPlayerBtnClicked);

    connect(m_closeBtn, &DIconButton::clicked,
            this, &VNotePlayWidget::onCloseBtnClicked);

    connect(m_slider, &DSlider::sliderPressed,
            this, &VNotePlayWidget::onSliderPressed);
    connect(m_slider, &DSlider::sliderReleased,
            this, &VNotePlayWidget::onSliderReleased);
    connect(m_slider, &DSlider::sliderMoved,
            this, &VNotePlayWidget::onSliderMove);
}

void VNotePlayWidget::onVoicePlayPosChange(qint64 pos)
{
    if(m_sliderReleased == true){
        onSliderMove(static_cast<int>(pos));
    }

    if (pos >=  m_slider->maximum()) {
        onCloseBtnClicked();
    }

}

void VNotePlayWidget::setVoiceBlock(VNVoiceBlock *voiceData)
{
    if (voiceData) {
        if (voiceData != m_voiceBlock) {
            stopVideo();
            m_voiceBlock = voiceData;
            m_player->setMedia(QUrl::fromLocalFile(m_voiceBlock->voicePath));
            m_nameLab->setText(voiceData->voiceTitle);
            m_timeLab->setText(Utils::formatMillisecond(0, 0) + "/" +
                               Utils::formatMillisecond(voiceData->voiceSize));
        }
        onPlayBtnClicked();
    }
}

void VNotePlayWidget::stopVideo()
{
    m_player->stop();
}
void VNotePlayWidget::playVideo()
{
    m_player->play();
}

void VNotePlayWidget::pauseVideo()
{
    m_player->pause();
}

void VNotePlayWidget::onPlayBtnClicked()
{
    m_playerBtn->setState(VNote2SIconButton::Press);
    onPlayerBtnClicked();
}

void VNotePlayWidget::onPauseBtnClicked()
{
    m_playerBtn->setState(VNote2SIconButton::Normal);
    onPlayerBtnClicked();
}

void VNotePlayWidget::onCloseBtnClicked()
{
    m_slider->setValue(0);
    stopVideo();
    m_sliderReleased = true;
    emit sigWidgetClose(m_voiceBlock);
}

void VNotePlayWidget::onSliderPressed()
{
    m_sliderReleased = false;
}

void VNotePlayWidget::onSliderReleased()
{
     m_sliderReleased = true;
     if(m_player->state() != QMediaPlayer::StoppedState){
         int pos = m_slider->value();
         if (pos >=  m_slider->maximum()) {
             onCloseBtnClicked();
         }else {
            m_player->setPosition(pos);
         }
     }
}

void VNotePlayWidget::onSliderMove(int pos)
{
    if(m_voiceBlock){
        qint64 tmpPos = pos > m_voiceBlock->voiceSize ? m_voiceBlock->voiceSize : pos;
        m_timeLab->setText(Utils::formatMillisecond(tmpPos, 0) + "/" +
                           Utils::formatMillisecond(m_voiceBlock->voiceSize));
    }

    if(m_sliderReleased == true){
       m_slider->setValue(pos);
    }
}

bool VNotePlayWidget::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o)
    if (e->type() == QEvent::Enter) {
        m_nameLab->setVisible(false);
    } else if (e->type() == QEvent::Leave) {
        m_nameLab->setVisible(true);
    }
    return  false;
}

VNVoiceBlock* VNotePlayWidget::getVoiceData()
{
    return m_voiceBlock;
}

QMediaPlayer::State VNotePlayWidget::getPlayerStatus()
{
    return m_player->state();
}

void VNotePlayWidget::onDurationChanged(qint64 duration)
{
    if(m_slider->maximum() != duration){
        m_slider->setMaximum(static_cast<int>(duration));
    }
}

void VNotePlayWidget::onPlayerBtnClicked()
{
    bool isPress = m_playerBtn->isPressed();
    if(isPress){
        playVideo();
        emit sigPlayVoice(m_voiceBlock);
    }else {
        pauseVideo();
        emit sigPauseVoice(m_voiceBlock);
    }
}
