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
#include <QTimer>

#include <DFontSizeManager>
#include <DStyle>
#include <DAnchors>

#define DefaultHeight 64

VoiceNoteItem::VoiceNoteItem(VNoteBlock *noteBlock, QWidget *parent)
    : DetailItemWidget(parent)
    , m_noteBlock(noteBlock)
{
    initUi();
    initConnection();
    updateData();
}

void VoiceNoteItem::initUi()
{
    this->setFixedHeight(DefaultHeight);
    m_bgWidget = new DFrame(this);
    m_bgWidget->setBackgroundRole(DPalette::ItemBackground);

    m_createTimeLab = new DLabel(m_bgWidget);
    DFontSizeManager::instance()->bind(m_createTimeLab, DFontSizeManager::T8);
    m_createTimeLab->setForegroundRole(DPalette::TextTips);

    m_asrText = new TextNoteEdit(m_bgWidget);
    DFontSizeManager::instance()->bind(m_asrText, DFontSizeManager::T8);
    m_asrText->setReadOnly(true);
    DStyle::setFocusRectVisible(m_asrText, false);
    DPalette pb = DApplicationHelper::instance()->palette(m_asrText);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    pb.setBrush(DPalette::Text, pb.color(DPalette::Highlight));
    m_asrText->setPalette(pb);
    m_asrText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_asrText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_asrText->document()->setDocumentMargin(15);
    m_asrText->setVisible(false);
    m_asrText->setContextMenuPolicy(Qt::NoContextMenu);

    m_voiceSizeLab = new DLabel(m_bgWidget);
    DFontSizeManager::instance()->bind(m_voiceSizeLab, DFontSizeManager::T8);

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

    m_voiceNameLab = new DLabel(m_bgWidget);
    DFontSizeManager::instance()->bind(m_voiceNameLab, DFontSizeManager::T6);
    m_voiceNameLab->setForegroundRole(DPalette::TextTitle);

    m_hornLab = new DLabel(m_bgWidget);
    m_hornLab->setMinimumSize(28,25);

    m_hornLab->setAlignment(Qt::AlignRight);
    DAnchorsBase buttonAnchor(m_hornLab);
    buttonAnchor.setAnchor(Qt::AnchorRight, m_bgWidget, Qt::AnchorRight);
    buttonAnchor.setAnchor(Qt::AnchorTop, m_bgWidget, Qt::AnchorTop);
    buttonAnchor.setTopMargin(10);
    buttonAnchor.setRightMargin(25);


    QGridLayout *playBtnLayout = new QGridLayout;
    playBtnLayout->addWidget(m_pauseBtn, 0, 0);
    playBtnLayout->addWidget(m_playBtn, 0, 0);
    playBtnLayout->setSizeConstraint(QLayout::SetNoConstraint);

    QVBoxLayout *nameLayout = new QVBoxLayout;
    nameLayout->addWidget(m_voiceNameLab, Qt::AlignLeft);
    nameLayout->addWidget(m_createTimeLab, Qt::AlignLeft);
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(0);

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addSpacing(35);
    rightLayout->addWidget(m_voiceSizeLab);
    rightLayout->setSizeConstraint(QLayout::SetNoConstraint);

    QHBoxLayout *itemLayout = new QHBoxLayout;
    itemLayout->addLayout(playBtnLayout);
    itemLayout->addLayout(nameLayout, 1);
    itemLayout->addLayout(rightLayout);
    itemLayout->setSizeConstraint(QLayout::SetNoConstraint);
    itemLayout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *bkLayout = new QVBoxLayout;
    bkLayout->addLayout(itemLayout);
    bkLayout->addWidget(m_asrText);
    bkLayout->setContentsMargins(0, 0, 20, 0);
    m_bgWidget->setLayout(bkLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_bgWidget);
    mainLayout->setContentsMargins(10, 0, 10, 0);
    showPlayBtn();

    this->setLayout(mainLayout);


}

void VoiceNoteItem::updateData()
{
    if (m_noteBlock != nullptr) {
        VNVoiceBlock *voiceBlock = m_noteBlock->ptrVoice;
        if (voiceBlock) {
            m_createTimeLab->setText(Utils::convertDateTime(voiceBlock->createTime));
            m_voiceNameLab->setText(voiceBlock->voiceTitle);
            m_voiceSizeLab->setText(Utils::formatMillisecond(voiceBlock->voiceSize));
            if (!m_noteBlock->blockText.isEmpty()) {
                showAsrEndWindow(m_noteBlock->blockText);
            }
        }
    }
}

void VoiceNoteItem::initConnection()
{
    connect(m_pauseBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onPauseBtnClicked);
    connect(m_playBtn, &VNoteIconButton::clicked, this, &VoiceNoteItem::onPlayBtnClicked);
    connect(m_asrText, &TextNoteEdit::textChanged, this, &VoiceNoteItem::onAsrTextChange);
    QTextDocument *document = m_asrText->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, [ = ] {
        onAsrTextChange();
    });
}

void VoiceNoteItem::onPlayBtnClicked()
{
    emit sigPlayBtnClicked(this);
}

void VoiceNoteItem::onPauseBtnClicked()
{
    emit sigPauseBtnClicked(this);
}

VNoteBlock *VoiceNoteItem::getNoteBlock()
{
    return  m_noteBlock;
}

void VoiceNoteItem::showPlayBtn()
{
    if(m_pauseBtn->isVisible()){
        stopAnim();
    }

    m_pauseBtn->setVisible(false);
    m_playBtn->setVisible(true);
    m_hornLab->setVisible(false);
}

void VoiceNoteItem::showPauseBtn()
{
    if(!m_pauseBtn->isVisible()){
         startAnim();
    }

    m_pauseBtn->setVisible(true);
    m_playBtn->setVisible(false);
    m_hornLab->setVisible(true);
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
    if (strResult.isEmpty()) {
        m_asrText->setVisible(false);
    } else {
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

void VoiceNoteItem::onAsrTextChange()
{
    int height = DefaultHeight;
    QTextDocument *doc = m_asrText->document();
    if (!doc->isEmpty()) {
        int docHeight = static_cast<int>(doc->size().height());
        m_asrText->setFixedHeight(docHeight);
        height += docHeight;
    }
    this->setFixedHeight(height);
}

bool VoiceNoteItem::asrTextNotEmpty()
{
    return m_noteBlock && !m_noteBlock->blockText.isEmpty();
}

void VoiceNoteItem::selectText(const QPoint &globalPos, QTextCursor::MoveOperation op)
{
    if(asrTextNotEmpty()){
       m_asrText->selectText(globalPos,op);
    }
}

void VoiceNoteItem::selectText(QTextCursor::MoveOperation op)
{
    if(asrTextNotEmpty()){
      m_asrText->moveCursor(op,QTextCursor::KeepAnchor);
    }
}

void VoiceNoteItem::selectAllText()
{
    if(asrTextNotEmpty()){
        m_asrText->selectAll();
    }
    m_select = true;
}

void VoiceNoteItem::clearSelection()
{
    if(asrTextNotEmpty()){
       m_asrText->clearSelection();
    }
    m_select = false;
}

QString VoiceNoteItem::getSelectText()
{
    QString ret = "";
    if(asrTextNotEmpty()){
        ret = m_asrText->getSelectText();
    }
    return  ret;
}

QString VoiceNoteItem::getAllText()
{
    QString ret = "";
    if(asrTextNotEmpty()){
        ret = m_asrText->toPlainText();
    }
    return  ret;
}

bool VoiceNoteItem::hasSelection()
{
    return  m_select || (asrTextNotEmpty() && m_asrText->hasSelection());
}

void VoiceNoteItem::removeSelectText()
{
    if(asrTextNotEmpty()){
        m_asrText->removeSelectText();
    }
}

QTextCursor VoiceNoteItem::getTextCursor()
{
    QTextCursor cursor;
    if(asrTextNotEmpty()){
        cursor = m_asrText->textCursor();
    }
    return  cursor;
}

void VoiceNoteItem::setTextCursor(const QTextCursor &cursor)
{
    if(asrTextNotEmpty()){
        m_asrText->setTextCursor(cursor);
    }
}

bool VoiceNoteItem::textIsEmpty()
{
    return  asrTextNotEmpty();
}

QRect VoiceNoteItem::getCursorRect()
{
    QRect rc;
    if(asrTextNotEmpty()){
        rc = m_asrText->cursorRect(m_asrText->textCursor());
    }
    return rc;
}

bool VoiceNoteItem::isAsrTextPos(const QPoint &globalPos)
{
    bool ret = false;
    if(asrTextNotEmpty()){
        QPoint pos = m_asrText->mapFromGlobal(globalPos);
        if(m_asrText->rect().contains(pos)){
            ret = true;
        }
    }
    return ret;
}

void VoiceNoteItem::setFocus()
{
    QWidget *parent = static_cast<QWidget*>(this->parent());
    if(parent){
        parent->setFocus();
    }
}

bool VoiceNoteItem::hasFocus()
{
    QWidget *parent = static_cast<QWidget*>(this->parent());
    if(parent){
        return parent->hasFocus();
    }
    return false;
}

bool VoiceNoteItem::isAsring()
{
    return  m_asrText->isVisible() && m_noteBlock->blockText.isEmpty();
}

void VoiceNoteItem::updateAnim()
{
    QPixmap playAnimPic = Utils::loadSVG(m_playBitmap[m_animPicIndex]);

    ++m_animPicIndex;
    m_animPicIndex = m_animPicIndex % m_playBitmap.size();

    if (nullptr != m_hornLab) {
        m_hornLab->setPixmap(playAnimPic);
    }
}

void VoiceNoteItem::stopAnim()
{
    PlayAnimInferface::stopAnim();

    //Reset null
    if (nullptr != m_hornLab) {
        m_hornLab->setPixmap(QPixmap());
    }
}

PlayAnimInferface::~PlayAnimInferface()
{

}

void PlayAnimInferface::startAnim()
{
    if (nullptr != m_refreshTimer) {
        if (!m_refreshTimer->isActive()) {
            m_refreshTimer->start();
        }
        //Reset index at start;
        m_animPicIndex = 0;
    }
}

void PlayAnimInferface::stopAnim()
{
    if (nullptr != m_refreshTimer) {
        if (m_refreshTimer->isActive()) {
            m_refreshTimer->stop();
        }
    }
}

void PlayAnimInferface::setAnimTimer(QTimer *timer)
{
    if (nullptr != timer) {
        m_refreshTimer = timer;
    }
}

void PlayAnimInferface::updateAnim()
{
}
