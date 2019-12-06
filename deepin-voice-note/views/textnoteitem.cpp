#include "textnoteitem.h"
#include "common/vnoteitem.h"
#include "common/utils.h"

#include <DFontSizeManager>
#include <DApplicationHelper>

TextNoteItem::TextNoteItem(VNoteItem *textNote,QWidget *parent)
    : QWidget(parent)
    , m_textNode(textNote)
{
    initUI();
    initData();
    initConnection();
}
void TextNoteItem::initUI()
{
    m_timeLabel = new DLabel(this);
    m_timeLabel->setFixedHeight(16);
    DFontSizeManager::instance()->bind(m_timeLabel,DFontSizeManager::T9);
    m_bgWidget = new DFrame(this);
    m_bgWidget->setObjectName("widget");
    m_bgWidget->setFixedHeight(140);

    DPalette pb = DApplicationHelper::instance()->palette(m_bgWidget);
    pb.setBrush(DPalette::Base, pb.color(DPalette::FrameBorder));
    m_bgWidget->setPalette(pb);
    m_itemLayout = new QVBoxLayout();
    m_itemLayout->setContentsMargins(0,0,0,0);
    m_itemLayout->addSpacing(6);
    m_itemLayout->addWidget(m_timeLabel);
    m_itemLayout->addSpacing(2);
    m_itemLayout->addWidget(m_bgWidget);
    m_itemLayout->addSpacing(6);
    this->setLayout(m_itemLayout);
    m_hBoxLayout = new QHBoxLayout(m_bgWidget);
    m_hBoxLayout->setContentsMargins(14, 0, 0, 0);
    m_hBoxLayout->setObjectName("horizontalLayout");

    m_textEdit = new DTextEdit(m_bgWidget);
    QTextCursor textCursor = m_textEdit->textCursor();
    QTextBlockFormat textBlockFormat;
    textBlockFormat.setLineHeight(24, QTextBlockFormat::FixedHeight);//设置固定行高
    textCursor.setBlockFormat(textBlockFormat);
    m_textEdit->setTextCursor(textCursor);
    DFontSizeManager::instance()->bind(m_textEdit,DFontSizeManager::T8);
    m_textEdit->setFixedHeight(133);
    m_hBoxLayout->addWidget(m_textEdit);
    m_hBoxLayout->addSpacing(13);
    QPalette pl = m_textEdit->palette();
    pl.setBrush(QPalette::Base,QBrush(QColor(0,0,0,0)));
    m_textEdit->setPalette(pl);
    m_textEdit->setFrameShape(QFrame::NoFrame);
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}
void TextNoteItem::initData()
{
    if(m_textNode != nullptr)
    {
        m_timeLabel->setText("  " + Utils::convertDateTime(m_textNode->createTime));
    }
}
void TextNoteItem::initConnection()
{
    ;
}
