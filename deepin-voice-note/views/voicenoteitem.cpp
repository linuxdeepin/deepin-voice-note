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
    //m_waveForm->setStyleSheet("background:red");

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

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        m_menuBtn = new MyRecodeButtons(
            ":/images/icon/normal/more_normal.svg", ":/images/icon/press/more_press.svg",
            ":/images/icon/hover/more_hover.svg", ":/images/icon/disabled/more_disabled.svg",
            ":/images/icon/focus/more_focus.svg", QSize(44, 44), this);
        m_pauseBtn = new MyRecodeButtons(
            ":/images/icon/normal/pause_blue_normal.svg",
            ":/images/icon/press/pause_blue_press.svg", ":/images/icon/hover/pause_blue_hover.svg",
            "", ":/images/icon/focus/pause_blue_focus.svg", QSize(60, 60), this);
        m_playBtn = new MyRecodeButtons(
            ":/images/icon/normal/play_normal.svg", ":/images/icon/press/play_press.svg",
            ":/images/icon/hover/play_hover.svg", ":/images/icon/disabled/play_disabled.svg",
            ":/images/icon/focus/play_focus.svg", QSize(60, 60), this);
        m_detailBtn = new MyRecodeButtons(
            ":/images/icon/normal/detail-normal.svg",
            ":/images/icon/press/detail-press.svg",
            ":/images/icon/hover/detail-hover.svg",
            "",
            "",
            QSize(44, 44),
            this);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        m_menuBtn = new MyRecodeButtons(":/images/icon_dark/normal/more_normal_dark.svg",
                                        ":/images/icon_dark/press/more_press_dark.svg",
                                        ":/images/icon_dark/hover/more_hover_dark.svg",
                                        ":/images/icon_dark/disabled/more_disabled_dark.svg",
                                        ":/images/icon_dark/focus/more_focus_dark.svg",
                                        QSize(44, 44), this);
        m_pauseBtn = new MyRecodeButtons(":/images/icon_dark/normal/pause_blue_normal_dark.svg",
                                         ":/images/icon_dark/press/pause_blue_press_dark.svg",
                                         ":/images/icon_dark/hover/pause_blue_hover_dark.svg", "",
                                         ":/images/icon_dark/focus/pause_blue_focus_dark.svg",
                                         QSize(60, 60), this);
        m_playBtn = new MyRecodeButtons(":/images/icon_dark/normal/play_normal_dark.svg",
                                        ":/images/icon_dark/press/play_press_dark.svg",
                                        ":/images/icon_dark/hover/play_hover_dark.svg",
                                        ":/images/icon_dark/disabled/play_disabled_dark.svg",
                                        ":/images/icon_dark/focus/play_focus_dark.svg",
                                        QSize(60, 60), this);
        m_detailBtn = new MyRecodeButtons(
            ":/images/icon_dark/normal/detail-normal.svg",
            ":/images/icon_dark/press/detail-press.svg",
            ":/images/icon_dark/hover/detail-hover.svg",
            "",
            "",
            QSize(44, 44),
            this);
    }
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
    m_voiceSizeLab->setText("00'00\"");
    if (!m_textNode->noteText.isEmpty()) {
        showAsrEndWindow(m_textNode->noteText);
    }
}

void VoiceNoteItem::initConnection()
{
    connect(m_menuBtn, &DPushButton::clicked, this, &VoiceNoteItem::onshowMenu);
    connect(m_pauseBtn, &MyRecodeButtons::clicked, this, &VoiceNoteItem::onPauseBtnClicked);
    connect(m_playBtn, &MyRecodeButtons::clicked, this, &VoiceNoteItem::onPlayBtnClicked);
    connect(m_detailBtn, &MyRecodeButtons::clicked, this, &VoiceNoteItem::onShowDetail);
    connect(m_asrText, &DTextEdit::textChanged, this, &VoiceNoteItem::onTextChanged);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VoiceNoteItem::onChangeTheme);
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
}

void VoiceNoteItem::showAsrEndWindow(const QString &strResult)
{
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
    if (enable) {
        m_playBtn->EnAbleBtn();
    } else {
        m_playBtn->DisableBtn();
    }
}

void VoiceNoteItem::enblePauseBtn(bool enable)
{
    if (enable) {
        m_pauseBtn->EnAbleBtn();
    } else {
        m_pauseBtn->DisableBtn();
    }
}

bool VoiceNoteItem::isPlaying()
{
    return  m_pauseBtn->isVisible();
}

bool VoiceNoteItem::isAsrStart()
{
    bool flag = false;
    if (m_asrText->isVisible()) {
        QString text = m_asrText->toPlainText();
        if (text == tr("Converting voice to text")) {
            flag = true;
        }
    }
    return flag;
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
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();

    if (themeType == DGuiApplicationHelper::LightType) {
        if (nullptr != m_menuBtn) {
            m_menuBtn->setPicChange(
                ":/images/icon/normal/more_normal.svg", ":/images/icon/press/more_press.svg",
                ":/images/icon/hover/more_hover.svg", ":/images/icon/disabled/more_disabled.svg",
                ":/images/icon/focus/more_focus.svg");
        }

        if (nullptr != m_detailBtn) {
            m_detailBtn->setPicChange(":/images/icon/normal/detail-normal.svg",
                                      ":/images/icon/press/detail-press.svg",
                                      ":/images/icon/hover/detail-hover.svg", "", "");
        }

    } else if (themeType == DGuiApplicationHelper::DarkType) {
        if (nullptr != m_menuBtn) {
            m_menuBtn->setPicChange(":/images/icon_dark/normal/more_normal_dark.svg",
                                    ":/images/icon_dark/press/more_press_dark.svg",
                                    ":/images/icon_dark/hover/more_hover_dark.svg",
                                    ":/images/icon_dark/disabled/more_disabled_dark.svg",
                                    ":/images/icon_dark/focus/more_focus_dark.svg");
        }
        if (nullptr != m_detailBtn) {
            m_detailBtn->setPicChange(":/images/icon_dark/normal/detail-normal.svg",
                                      ":/images/icon_dark/press/detail-press.svg",
                                      ":/images/icon_dark/hover/detail-hover.svg", "", "");
        }
    }

}
