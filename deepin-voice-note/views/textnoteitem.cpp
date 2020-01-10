#include "textnoteitem.h"
#include "common/utils.h"

#include <QDebug>
#include <QGridLayout>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTimer>

#include <DApplicationHelper>
#include <DFileDialog>
#include <DFontSizeManager>
#include <DMessageBox>

TextNoteItem::TextNoteItem(VNoteItem *textNote, QWidget *parent)
    : VNoteItemWidget(parent)
    , m_textNode(textNote)
{
    initUI();
    initConnection();
    updateData();
}

void TextNoteItem::initUI()
{
    m_timeLabel = new DLabel(this);
    m_timeLabel->setFixedHeight(16);
    DFontSizeManager::instance()->bind(m_timeLabel, DFontSizeManager::T9);

    m_textEdit = new TextNoteEdit(this);
    DFontSizeManager::instance()->bind(m_textEdit, DFontSizeManager::T8);
    m_textEdit->setFixedHeight(140);
    m_textEdit->setLineWidth(0);
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEdit->document()->setDocumentMargin(10);
    m_textEdit->setFrameShape(QFrame::NoFrame);
    m_textEditFormat =  m_textEdit->currentCharFormat();
    DPalette pb = DApplicationHelper::instance()->palette(this);
    pb.setBrush(DPalette::Text,pb.color(DPalette::Active,DPalette::WindowText));
    pb.setBrush(DPalette::Button, pb.color(DPalette::ItemBackground));
    this->setPalette(pb);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        m_menuBtn = new MyRecodeButtons(
            ":/images/icon/normal/more_normal.svg", ":/images/icon/press/more_press.svg",
            ":/images/icon/hover/more_hover.svg", ":/images/icon/disabled/more_disabled.svg",
            ":/images/icon/focus/more_focus.svg", QSize(44, 44), this);
        m_detailBtn = new MyRecodeButtons(
            ":/images/icon/normal/detail-normal.svg", ":/images/icon/press/detail-press.svg",
            ":/images/icon/hover/detail-hover.svg", "", "", QSize(44, 44), this);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        m_menuBtn = new MyRecodeButtons(":/images/icon_dark/normal/more_normal_dark.svg",
                                        ":/images/icon_dark/press/more_press_dark.svg",
                                        ":/images/icon_dark/hover/more_hover_dark.svg",
                                        ":/images/icon_dark/disabled/more_disabled_dark.svg",
                                        ":/images/icon_dark/focus/more_focus_dark.svg",
                                        QSize(44, 44), this);
        m_detailBtn = new MyRecodeButtons(":/images/icon_dark/normal/detail-normal.svg",
                                          ":/images/icon_dark/press/detail-press.svg",
                                          ":/images/icon_dark/hover/detail-hover.svg", "", "",
                                          QSize(44, 44), this);
    }
    m_detailBtn->setVisible(false);

    QVBoxLayout *btnLayout = new QVBoxLayout;
    btnLayout->setContentsMargins(0, 15, 10, 0);
    btnLayout->setSpacing(0);
    btnLayout->setSizeConstraint(QLayout::SetNoConstraint);
    btnLayout->addWidget(m_menuBtn);
    btnLayout->addSpacing(23);
    btnLayout->addWidget(m_detailBtn);

    QGridLayout *bglayout = new QGridLayout;
    bglayout->addWidget(m_timeLabel, 0, 0);
    bglayout->addWidget(m_textEdit, 1, 0, 3, 3);
    bglayout->addLayout(btnLayout, 1, 2, 1, 1);
    bglayout->setRowStretch(0, 1);
    bglayout->setRowStretch(1, 1);
    bglayout->setRowStretch(2, 0);
    bglayout->setColumnStretch(0, 1);
    bglayout->setColumnStretch(1, 1);
    bglayout->setColumnStretch(2, 0);
    bglayout->setMargin(0);
    bglayout->setSizeConstraint(QLayout::SetNoConstraint);
    bglayout->setContentsMargins(0, 6, 0, 0);
    this->setLayout(bglayout);

}

void TextNoteItem::updateData()
{
    if (m_textNode != nullptr) {
        m_timeLabel->setText("  " + Utils::convertDateTime(m_textNode->createTime));
        m_textEdit->setText(m_textNode->noteText);
    }
}

void TextNoteItem::initConnection()
{
    connect(m_textEdit, &TextNoteEdit::textChanged, this, &TextNoteItem::onTextChanged);
    connect(m_textEdit, &TextNoteEdit::sigFocusIn, this, &TextNoteItem::onEditFocusIn);
    connect(m_textEdit, &TextNoteEdit::sigFocusOut, this, &TextNoteItem::onEditFocusOut);
    connect(m_detailBtn, &DPushButton::clicked, this, &TextNoteItem::onShowDetail);
    connect(m_menuBtn, &DPushButton::clicked, this, &TextNoteItem::onShowMenu);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &TextNoteItem::onChangeTheme);
}

void TextNoteItem::onTextChanged()
{
    adjustTextEdit();
    updateDetilBtn();
    emit sigTextEditIsEmpty(m_textNode, m_textEdit->document()->isEmpty());
}

void TextNoteItem::onShowDetail()
{
    emit sigTextEditDetail(m_textNode, m_textEdit, m_searchKey);
}

void TextNoteItem::onEditFocusIn()
{
    qDebug() << "edit focus in";
}

void TextNoteItem::onEditFocusOut()
{
    QString text = m_textEdit->toPlainText();
    if (text.isEmpty()) {
        emit sigDelNote(m_textNode);
    } else {
        if (text != m_textNode->noteText) {
            m_textNode->noteText = text;
            emit sigUpdateNote(m_textNode);
        }
    }
    qDebug() << "id:" << m_textNode->noteId << "fouce out";
}

void TextNoteItem::onShowMenu()
{
    emit sigMenuPopup(m_textNode);
}

void TextNoteItem::highSearchText(const QRegExp &searchKey, const QColor &highColor)
{
    if (!searchKey.isEmpty()) {
        if (Utils::highTextEdit(m_textEdit, m_textEditFormat, searchKey, highColor)) {
            m_searchKey = searchKey;
        }
    }
}

VNoteItem *TextNoteItem::getNoteItem()
{
    return m_textNode;
}

void TextNoteItem::changeToEdit()
{
    m_textEdit->setFocus();
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_textEdit->setTextCursor(cursor);
}
void TextNoteItem::onChangeTheme()
{
    DPalette pb = DApplicationHelper::instance()->palette(m_textEdit);
    pb.setBrush(DPalette::Button, pb.color(DPalette::ItemBackground));
    pb.setBrush(DPalette::Text, pb.color(DPalette::Active, DPalette::WindowText));
    m_textEdit->setPalette(pb);

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
void TextNoteItem::resizeEvent(QResizeEvent *event)
{
    updateDetilBtn();
    VNoteItemWidget::resizeEvent(event);
}

void TextNoteItem::leaveEvent(QEvent *event)
{
    VNoteItemWidget::leaveEvent(event);
    DPalette pb = DApplicationHelper::instance()->palette(m_textEdit);
    pb.setBrush(DPalette::Text,pb.color(DPalette::Active,DPalette::WindowText));
    pb.setBrush(DPalette::Button, pb.color(DPalette::ItemBackground));
    m_textEdit->setPalette(pb);
}

void TextNoteItem::enterEvent(QEvent *event)
{
    VNoteItemWidget::enterEvent(event);
    DPalette pb = DApplicationHelper::instance()->palette(m_textEdit);
    pb.setBrush(DPalette::Text,pb.color(DPalette::Active,DPalette::WindowText));
    pb.setBrush(DPalette::Button, pb.color(DPalette::Light));
    m_textEdit->setPalette(pb);
}

void TextNoteItem::adjustTextEdit()
{
    QTextDocument *document = m_textEdit->document();
    for (QTextBlock it = document->begin(); it != document->end(); it = it.next()) {
        QTextCursor textCursor(it);
        QTextBlockFormat textBlockFormat = it.blockFormat();
        int right = static_cast<int>(textBlockFormat.rightMargin());
        int height = static_cast<int>(textBlockFormat.lineHeight());
        if (right != 50 || height != 26) {
            textBlockFormat.setRightMargin(50);
            textBlockFormat.setLineHeight(26, QTextBlockFormat::FixedHeight);
            textCursor.setBlockFormat(textBlockFormat);
        }
    }
}

void TextNoteItem::updateDetilBtn()
{
    QTimer::singleShot(0, this, [ = ] {
        int textEditHeight = m_textEdit->height();
        QTextDocument *document = m_textEdit->document();
        int documentHeight = static_cast<int>(document->size().height());
        if (textEditHeight < documentHeight - 10){
            m_detailBtn->setVisible(true);
        } else{
            m_detailBtn->setVisible(false);
        }
    });
}
