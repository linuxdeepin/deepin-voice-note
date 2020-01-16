#include "voicenoteitem.h"
#include "common/vnoteitem.h"
#include "common/utils.h"

#include <QDebug>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QTextBlock>

#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DStyle>

#define ASRTEXTBOTTOMSPACE 15
#define MAXTEXTEDITHEIGHT 140

VoiceNoteItem::VoiceNoteItem(VNoteItem *textNote, QWidget *parent)
    : VNoteItemWidget(parent)
    , m_textNode(textNote)
{
    initUi();
    initConnection();
    initData();
    onTextChanged();
}

void VoiceNoteItem::initUi()
{
    m_createTimeLab = new DLabel(this);
    m_createTimeLab->setFixedHeight(16);
    DFontSizeManager::instance()->bind(m_createTimeLab, DFontSizeManager::T9);
    m_bgWidget = new DFrame(this);
    m_bgWidget->setLineWidth(0);
    DPalette pb = DApplicationHelper::instance()->palette(m_bgWidget);
    pb.setBrush(DPalette::Base, pb.color(DPalette::ItemBackground));
    m_bgWidget->setPalette(pb);

    m_waveForm = new VNWaveform(this);
    m_waveForm->setFixedHeight(33);
    m_waveForm->setEnabled(false);

    m_asrText = new TextNoteEdit(this);
    DFontSizeManager::instance()->bind(m_asrText, DFontSizeManager::T8);
    m_asrText->setReadOnly(true);
    DStyle::setFocusRectVisible(m_asrText, false);
    pb = DApplicationHelper::instance()->palette(m_asrText);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    pb.setBrush(DPalette::Text, pb.color(DPalette::Highlight));
    m_asrText->setPalette(pb);
    m_asrText->document()->setDocumentMargin(5);
    m_asrText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_asrText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_voiceSizeLab = new DLabel(this);
    m_voiceSizeLab->setFixedSize(66, 30);
    m_voiceSizeLab->setAlignment(Qt::AlignCenter);

    m_curSizeLab = new DLabel(this);
    m_curSizeLab->setFixedSize(66, 30);
    m_curSizeLab->setAlignment(Qt::AlignCenter);

    m_menuBtn = new VNoteIconButton(this
                                    , "more_normal.svg"
                                    , "more_hover.svg"
                                    , "more_press.svg");
    m_menuBtn->setIconSize(QSize(44, 44));
    m_menuBtn->setFlat(true);

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

    m_detailBtn = new VNoteIconButton(this
                                      , "detail_normal.svg"
                                      , "detail_press.svg"
                                      , "detail_hover.svg");
    m_detailBtn->setIconSize(QSize(44, 44));
    m_detailBtn->setFlat(true);

    QGridLayout *mainLayout = new QGridLayout;
    QVBoxLayout *createTimeLayout = new QVBoxLayout;
    createTimeLayout->addWidget(m_createTimeLab);
    createTimeLayout->setContentsMargins(0, 0, 0, 10);
    mainLayout->addLayout(createTimeLayout, 0, 0, 1, 1);
    mainLayout->addWidget(m_bgWidget, 1, 0, 3, 3);

    QGridLayout *playBtnLayout = new QGridLayout;
    playBtnLayout->addWidget(m_pauseBtn, 0, 0);
    playBtnLayout->addWidget(m_playBtn, 0, 0);
    playBtnLayout->setContentsMargins(5, 2, 0, 2);
    mainLayout->addLayout(playBtnLayout, 1, 0);

    QHBoxLayout *layoutH = new QHBoxLayout;
    layoutH->addWidget(m_curSizeLab);
    layoutH->addWidget(m_waveForm);
    layoutH->addWidget(m_voiceSizeLab);
    layoutH->addSpacing(6);
    layoutH->addWidget(m_menuBtn);
    layoutH->setSpacing(0);
    layoutH->setContentsMargins(0, 0, 0, 0);
    layoutH->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(layoutH, 1, 1, 1, 2);
    mainLayout->setRowStretch(0, 0);
    mainLayout->setRowStretch(1, 1);
    mainLayout->setRowStretch(2, 0);

    mainLayout->setColumnStretch(0, 0);
    mainLayout->setColumnStretch(1, 1);
    mainLayout->setColumnStretch(2, 0);

    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 5, 0, 0);
    this->setLayout(mainLayout);
    m_detailBtn->setVisible(false);
    m_asrText->setVisible(false);
    m_pauseBtn->setVisible(false);
}

void VoiceNoteItem::initData()
{
    m_createTimeLab->setText(Utils::convertDateTime(m_textNode->createTime));
    m_voiceSizeLab->setText(Utils::formatMillisecond(m_textNode->voiceSize));
    m_curSizeLab->setText(Utils::formatMillisecond(0,0));
    if (!m_textNode->noteText.isEmpty()) {
        showAsrEndWindow(m_textNode->noteText);
    }
    m_waveForm->setMinimum(0);
    m_waveForm->setMaximum(static_cast<int>(m_textNode->voiceSize));
}

void VoiceNoteItem::initConnection()
{
    connect(m_menuBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onshowMenu);
    connect(m_pauseBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onPauseBtnClicked);
    connect(m_playBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onPlayBtnClicked);
    connect(m_detailBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onShowDetail);
    connect(m_asrText, &DTextEdit::textChanged, this, &VoiceNoteItem::onTextChanged);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VoiceNoteItem::onChangeTheme);
    connect(m_waveForm,&VNWaveform::sliderReleased,this,&VoiceNoteItem::onSliderReleased);
    connect(m_waveForm,&VNWaveform::sliderPressed,this,&VoiceNoteItem::onSliderPressed);
}

void VoiceNoteItem::onshowMenu()
{
    emit sigMenuPopup(m_textNode);
}

void VoiceNoteItem::onShowDetail()
{
    emit sigTextEditDetail(m_textNode, m_asrText, QRegExp());
}

void VoiceNoteItem::onPlayBtnClicked()
{
    emit sigVoicePlayBtnClicked(this);
}

void VoiceNoteItem::onPauseBtnClicked()
{
    emit sigVoicePauseBtnClicked(this);
}


VNoteItem *VoiceNoteItem::getNoteItem()
{
    return m_textNode;
}

void VoiceNoteItem::showPlayBtn()
{
    m_pauseBtn->setVisible(false);
    m_playBtn->setVisible(true);
}

void VoiceNoteItem::showPauseBtn()
{
    m_pauseBtn->setVisible(true);
    m_playBtn->setVisible(false);
}

void VoiceNoteItem::showAsrStartWindow()
{
    QTextOption option = m_asrText->document()->defaultTextOption();
    option.setAlignment(Qt::AlignCenter);
    m_asrText->document()->setDefaultTextOption(option);
    m_asrText->setText(tr("Converting voice to text"));
    m_asrText->setVisible(true);
    m_isVoiceAsring = true;
}

void VoiceNoteItem::showAsrEndWindow(const QString &strResult)
{
    m_isVoiceAsring = false;
    m_asrText->setText(strResult);
    if (strResult.isEmpty()) {
        m_asrText->setVisible(false);
        m_detailBtn->setVisible(false);
        if (m_isBottomSpace == true) {
            m_isBottomSpace = false;
        }
        m_lastHeight = 0;
        emit sigItemAddHeight(95 - this->height());
        return;
    }
    QTextOption option = m_asrText->document()->defaultTextOption();
    option.setAlignment(Qt::AlignLeft);
    m_asrText->document()->setDefaultTextOption(option);
    m_asrText->setVisible(true);
}

void VoiceNoteItem::enblePlayBtn(bool enable)
{
    m_playBtn->setEnabled(enable);
}

void VoiceNoteItem::enblePauseBtn(bool enable)
{
    m_pauseBtn->setEnabled(enable);
}

bool VoiceNoteItem::isPlaying()
{
    return  m_pauseBtn->isVisible();
}

bool VoiceNoteItem::isAsrStart()
{
    return m_isVoiceAsring;
}
void VoiceNoteItem::onTextChanged()
{
    setTextLineHeight(27);
    updateDetilBtn();
}

void VoiceNoteItem::resizeEvent(QResizeEvent *event)
{
    VNoteItemWidget::resizeEvent(event);
    if (m_detailBtn->isVisible()) {
        m_detailBtn->move(this->width() - 45, this->height() - 55);
    }
    if (m_asrText->isVisible()) {
        m_asrText->setFixedSize(QSize(this->width() - 70, this->height() - 110));
        m_asrText->move(9, 90);
    }
    updateDetilBtn();
}

void VoiceNoteItem::setTextLineHeight(int value)
{
    QTextDocument *document = m_asrText->document();
    for (QTextBlock it = document->begin(); it != document->end(); it = it.next()) {
        QTextCursor textCursor(it);
        QTextBlockFormat textBlockFormat = it.blockFormat();
        int height = static_cast<int>(textBlockFormat.lineHeight());
        if (height != value) {
            textBlockFormat.setLineHeight(value, QTextBlockFormat::FixedHeight);
            textCursor.setBlockFormat(textBlockFormat);
        }
    }
}

void VoiceNoteItem::updateDetilBtn()
{
    QTimer::singleShot(0, this, [ = ] {
        QTextDocument *doc = m_asrText->document();
        int documentHeight = 0;
        if (!doc->isEmpty())
        {
            documentHeight = static_cast<int>(doc->size().height());
        }
        if (documentHeight >= MAXTEXTEDITHEIGHT)
        {
            documentHeight = MAXTEXTEDITHEIGHT;
            m_detailBtn->setVisible(true);
        } else
        {
            m_detailBtn->setVisible(false);
        }
        if (documentHeight && m_lastHeight != documentHeight)
        {
            int value = documentHeight - m_lastHeight;
            if (m_isBottomSpace == false) {
                value += ASRTEXTBOTTOMSPACE;
                m_isBottomSpace = true;
            }
            emit sigItemAddHeight(value);
            m_lastHeight = documentHeight;
            qDebug() << m_lastHeight;
        }
    });
}

void VoiceNoteItem::onChangeTheme()
{
    DPalette pb = DApplicationHelper::instance()->palette(m_bgWidget);
    pb.setBrush(DPalette::Base, pb.color(DPalette::ItemBackground));
    m_bgWidget->setPalette(pb);
}

void VoiceNoteItem::onSliderReleased()
{
    emit sigVoicePlayPosChange(m_waveForm->value());
    m_isSliderPressed = false;
}

void VoiceNoteItem::onSliderPressed()
{
    m_isSliderPressed = true;
}
void VoiceNoteItem::setPlayPos(int pos)
{
    if(!m_isSliderPressed){
        m_waveForm->setValue(pos);
        if(m_waveForm->maximum() < 1000){
            m_curSizeLab->setText(Utils::formatMillisecond(pos,1));
        }else {
            m_curSizeLab->setText(Utils::formatMillisecond(pos,0));
        }
    }
}

void VoiceNoteItem::setSliderEnable(bool enable)
{
    m_waveForm->setEnabled(enable);
}

void VoiceNoteItem::enterEvent(QEvent *event)
{
    DPalette pb = DApplicationHelper::instance()->palette(m_bgWidget);
    pb.setBrush(DPalette::Base, pb.color(DPalette::Light));
    m_bgWidget->setPalette(pb);
    return VNoteItemWidget::enterEvent(event);
}

void VoiceNoteItem::leaveEvent(QEvent *event)
{
    DPalette pb = DApplicationHelper::instance()->palette(m_bgWidget);
    pb.setBrush(DPalette::Base, pb.color(DPalette::ItemBackground));
    m_bgWidget->setPalette(pb);
    return VNoteItemWidget::leaveEvent(event);
}
