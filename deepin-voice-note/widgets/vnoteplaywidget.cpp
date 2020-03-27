#include "vnoteplaywidget.h"
#include "vnoteiconbutton.h"
#include "common/vnoteitem.h"
#include "common/utils.h"

#include <QGridLayout>

VNotePlayWidget::VNotePlayWidget(QWidget *parent)
    : DFloatingWidget(parent)
{
    initUI();
    initPlayer();
    initConnection();
}

void VNotePlayWidget::initUI()
{
    m_pauseBtn = new VNoteIconButton(this
                                     , "pause_normal.svg"
                                     , "pause_hover.svg"
                                     , "pause_press.svg");
    m_pauseBtn->setIconSize(QSize(60, 60));
    m_pauseBtn->setFlat(true);
    m_playBtn = new VNoteIconButton(this
                                    , "play_normal.svg"
                                    , "play_hover.svg"
                                    , "play_press.svg");
    m_playBtn->setIconSize(QSize(60, 60));
    m_playBtn->setFlat(true);
    m_playBtn->SetDisableIcon("play_disabled.svg");

    m_timeLab = new DLabel(this);
    m_timeLab->setText("00:00/00:00");
    m_timeLab->setFixedWidth(124);
    m_sliderHover = new DWidget(this);
    m_nameLab = new DLabel(m_sliderHover);
    m_slider = new DSlider(Qt::Horizontal, m_sliderHover);
    m_slider->setMinimum(0);
    m_slider->setValue(0);

    m_closeBtn = new DIconButton(DStyle::SP_CloseButton, this);
    m_closeBtn->setIconSize(QSize(26, 26));
    m_closeBtn->setFlat(true);
    m_closeBtn->setFocusPolicy(Qt::ClickFocus);
    QGridLayout *playBtnLayout = new QGridLayout;
    playBtnLayout->addWidget(m_pauseBtn, 0, 0);
    playBtnLayout->addWidget(m_playBtn, 0, 0);
    m_playBtn->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *sliderLayout = new QVBoxLayout;
    sliderLayout->addWidget(m_nameLab, Qt::AlignLeft);
    sliderLayout->addWidget(m_slider);
    m_sliderHover->setLayout(sliderLayout);
    sliderLayout->setContentsMargins(0, 5, 0, 5);
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addLayout(playBtnLayout);
    mainLayout->addWidget(m_sliderHover);
    mainLayout->addWidget(m_timeLab, 0, Qt::AlignLeft);
    mainLayout->addWidget(m_closeBtn);
    mainLayout->setContentsMargins(5, 0, 10, 0);
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

    connect(m_playBtn, &VNoteIconButton::clicked,
            this, &VNotePlayWidget::onPlayBtnClicked);
    connect(m_pauseBtn, &VNoteIconButton::clicked,
            this, &VNotePlayWidget::onPauseBtnClicked);
    connect(m_closeBtn, &DIconButton::clicked,
            this, &VNotePlayWidget::onCloseBtnClicked);
    connect(m_slider, &DSlider::valueChanged,
            this, &VNotePlayWidget::onSliderValueChange);
}

void VNotePlayWidget::onVoicePlayPosChange(qint64 pos)
{

    qint64 tmpPos = pos > m_voiceBlock->voiceSize ? m_voiceBlock->voiceSize : pos;
    m_timeLab->setText(Utils::formatMillisecond(tmpPos, 0) + "/" +
                       Utils::formatMillisecond(m_voiceBlock->voiceSize));

    m_slider->setValue(static_cast<int>(pos));

    if (m_voiceBlock && pos >=  m_slider->maximum()) {
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
    m_playBtn->setVisible(false);
    m_pauseBtn->setVisible(true);
    playVideo();
    emit sigPlayVoice(m_voiceBlock);
}

void VNotePlayWidget::onPauseBtnClicked()
{
    m_playBtn->setVisible(true);
    m_pauseBtn->setVisible(false);
    pauseVideo();
    emit sigPauseVoice(m_voiceBlock);
}

void VNotePlayWidget::onCloseBtnClicked()
{
    m_slider->setValue(0);
    stopVideo();
    emit sigWidgetClose(m_voiceBlock);
}

void VNotePlayWidget::onSliderValueChange(int value)
{
    m_player->setPosition(value);
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
