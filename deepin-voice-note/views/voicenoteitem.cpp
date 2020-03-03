#include "voicenoteitem.h"
#include "textnoteedit.h"

#include "common/vnoteitem.h"
#include "common/utils.h"
#include "common/actionmanager.h"

#include "widgets/vnoteiconbutton.h"

#include <QDebug>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAbstractTextDocumentLayout>

#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DStyle>

#define DefaultHeight 64

VoiceNoteItem::VoiceNoteItem(VNoteItem *textNote, VNVoiceBlock *noteBlock, QWidget *parent)
    : DWidget(parent)
    , m_textNode(textNote)
    , m_noteBlock(noteBlock)
{
    initUi();
    initConnection();
    initData();
    onChangeTheme();
}

void VoiceNoteItem::initUi()
{
    this->setFixedHeight(DefaultHeight);
    m_bgWidget = new DFrame(this);
    m_createTimeLab = new DLabel(m_bgWidget);
    DFontSizeManager::instance()->bind(m_createTimeLab, DFontSizeManager::T8);
    m_createTimeLab->setText("2019-10-21 19:30");
    m_asrText = new TextNoteEdit(nullptr,nullptr,m_bgWidget);
    DFontSizeManager::instance()->bind(m_asrText, DFontSizeManager::T8);
    m_asrText->setReadOnly(true);
    DStyle::setFocusRectVisible(m_asrText, false);
    DPalette pb = DApplicationHelper::instance()->palette(m_asrText);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    pb.setBrush(DPalette::Text, pb.color(DPalette::Highlight));
    m_asrText->setPalette(pb);
    m_asrText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_asrText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_asrText->document()->setDocumentMargin(0);
    m_asrText->setVisible(false);
    m_voiceSizeLab = new DLabel(m_bgWidget);
    m_voiceSizeLab->setFixedSize(66, 30);
    m_voiceSizeLab->setAlignment(Qt::AlignCenter);
    m_pauseBtn = new VNoteIconButton(m_bgWidget
                                     , "pause_normal.svg"
                                     , "pause_hover.svg"
                                     , "pause_press.svg");
    m_pauseBtn->setIconSize(QSize(60, 60));
    m_pauseBtn->setFlat(true);

    m_playBtn = new VNoteIconButton(m_bgWidget
                                    , "play_normal.svg"
                                    , "play_hover.svg"
                                    , "play_press.svg");
    m_playBtn->setIconSize(QSize(60, 60));
    m_playBtn->setFlat(true);
    m_playBtn->SetDisableIcon("play_disabled.svg");
    m_voiceNameLab = new DLabel(this);
    DFontSizeManager::instance()->bind(m_voiceNameLab, DFontSizeManager::T6);
    QGridLayout *playBtnLayout = new QGridLayout;
    playBtnLayout->addWidget(m_pauseBtn,0,0);
    playBtnLayout->addWidget(m_playBtn,0,0);
    playBtnLayout->setSizeConstraint(QLayout::SetNoConstraint);
    QVBoxLayout *nameLayout = new QVBoxLayout;
    nameLayout->addWidget(m_voiceNameLab, Qt::AlignLeft);
    nameLayout->addWidget(m_createTimeLab, Qt::AlignLeft);
    nameLayout->setContentsMargins(0,0,0,0);
    nameLayout->setSpacing(0);
    m_voiceSizeLab->setText("50'00");
    QHBoxLayout *itemLayout = new QHBoxLayout;
    itemLayout->addLayout(playBtnLayout);
    itemLayout->addLayout(nameLayout,1);
    itemLayout->addWidget(m_voiceSizeLab);
    itemLayout->setSizeConstraint(QLayout::SetNoConstraint);
    itemLayout->setContentsMargins(0,0,0,0);
    QVBoxLayout *bkLayout = new QVBoxLayout;
    bkLayout->addLayout(itemLayout);
    bkLayout->addWidget(m_asrText);
    bkLayout->setContentsMargins(0,0,0,0);
    m_bgWidget->setLayout(bkLayout);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_bgWidget);
    mainLayout->setContentsMargins(10,0,10,0);
    showPlayBtn();
    m_voiceMenu = ActionManager::getVoiceRMenu(this);
    this->setLayout(mainLayout);


}

void VoiceNoteItem::initData()
{
    if(m_noteBlock != nullptr){
        m_createTimeLab->setText(Utils::convertDateTime(m_noteBlock->createTime));
        m_voiceNameLab->setText(m_noteBlock->voiceTitle);
        m_voiceSizeLab->setText(Utils::formatMillisecond(m_noteBlock->voiceSize));
    }
}

void VoiceNoteItem::initConnection()
{
    connect(m_pauseBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onPauseBtnClicked);
    connect(m_playBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onPlayBtnClicked);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VoiceNoteItem::onChangeTheme);
    //QTextDocument *document = m_asrText->document();
    //QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    //connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, &VoiceNoteItem::onAsrTextChange);
    connect(m_voiceMenu, SIGNAL(triggered(QAction *)),
            this, SIGNAL(sigAction(QAction *)));
}

void VoiceNoteItem::onPlayBtnClicked()
{
    showPauseBtn();
    emit sigPlayBtnClicked(this);
}

void VoiceNoteItem::onPauseBtnClicked()
{
    showPlayBtn();
    emit sigPauseBtnClicked(this);
}


VNoteItem *VoiceNoteItem::getNoteItem()
{
    return m_textNode;
}

VNVoiceBlock *VoiceNoteItem::getNoteBlock()
{
    return  m_noteBlock;
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
    m_asrText->setPlainText(DApplication::translate("VoiceNoteItem", "Converting voice to text"));
    m_asrText->setVisible(true);
    onAsrTextChange();
}

void VoiceNoteItem::showAsrEndWindow(const QString &strResult)
{
    m_asrText->setPlainText(strResult);
    QTextOption option = m_asrText->document()->defaultTextOption();
    option.setAlignment(Qt::AlignLeft);
    m_asrText->document()->setDefaultTextOption(option);
    if(strResult.isEmpty()){
        m_asrText->setVisible(false);
    }else {
        m_asrText->setVisible(true);
    }
    onAsrTextChange();
}

void VoiceNoteItem::enblePlayBtn(bool enable)
{
    m_playBtn->setBtnDisabled(!enable);
}

void VoiceNoteItem::enblePauseBtn(bool enable)
{
    m_pauseBtn->setEnabled(enable);
}

void VoiceNoteItem::onChangeTheme()
{
    DPalette pb = DApplicationHelper::instance()->palette(m_bgWidget);
    pb.setBrush(DPalette::Base, pb.color(DPalette::ItemBackground));
    m_bgWidget->setPalette(pb);
}

void VoiceNoteItem::onAsrTextChange()
{
    int height = DefaultHeight;
    QTextDocument *doc = m_asrText->document();
    if(!doc->isEmpty()){
        int docHeight = static_cast<int>(doc->size().height());
        m_asrText->setFixedHeight(docHeight);
        height += docHeight;
    }
    this->setFixedHeight(height);
}

void VoiceNoteItem::mousePressEvent(QMouseEvent *event)
{
    DWidget::mousePressEvent(event);
    if (event->button() == Qt::RightButton) {
        if(!m_asrText->geometry().contains(event->pos())){
           m_voiceMenu->exec(event->globalPos());
        }
    }
}
