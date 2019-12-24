#include "textnoteitem.h"
#include "common/utils.h"
#include "common/vnoteitem.h"

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
    DFontSizeManager::instance()->bind(m_timeLabel, DFontSizeManager::T9);

    m_bgWidget = new DFrame(this);
    m_bgWidget->setFixedHeight(140);
    DPalette pb = DApplicationHelper::instance()->palette(m_bgWidget);
    pb.setBrush(DPalette::Base, pb.color(DPalette::ItemBackground));
    m_bgWidget->setPalette(pb);

    m_textEdit = new TextNoteEdit(this);
    DFontSizeManager::instance()->bind(m_textEdit, DFontSizeManager::T8);
    m_textEdit->setFixedHeight(133);
    m_textEdit->setLineWidth(0);
    pb = DApplicationHelper::instance()->palette(m_textEdit);
    pb.setBrush(DPalette::Base, QColor(0, 0, 0, 0));
    m_textEdit->setPalette(pb);
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEdit->document()->setDocumentMargin(1);
    m_textEdit->setFrameShape(QFrame::NoFrame);
    m_textEditFormat = m_textEdit->currentCharFormat();

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
    m_contextMenu = new DMenu(this);
    m_saveAsAction = new QAction(tr("Save As TXT"), this);
    m_delAction = new QAction(tr("Delete"), this);
    m_contextMenu->addAction(m_saveAsAction);
    m_contextMenu->addAction(m_delAction);

    QVBoxLayout *btnLayout = new QVBoxLayout;
    btnLayout->setContentsMargins(0, 10, 10, 0);
    btnLayout->setSizeConstraint(QLayout::SetNoConstraint);
    btnLayout->addWidget(m_menuBtn);
    btnLayout->addSpacing(30);
    btnLayout->addWidget(m_detailBtn);

    QGridLayout *bglayout = new QGridLayout;
    bglayout->addWidget(m_timeLabel, 0, 0);
    bglayout->addWidget(m_bgWidget, 1, 0, 3, 3);
    bglayout->addWidget(m_textEdit, 1, 0, 3, 2);
    bglayout->addLayout(btnLayout, 1, 2, 1, 1);
    bglayout->setRowStretch(0, 1);
    bglayout->setRowStretch(1, 1);
    bglayout->setRowStretch(2, 0);
    bglayout->setColumnStretch(0, 1);
    bglayout->setColumnStretch(1, 1);
    bglayout->setColumnStretch(2, 0);
    bglayout->setMargin(0);

    bglayout->setSizeConstraint(QLayout::SetNoConstraint);
    this->setLayout(bglayout);
}

void TextNoteItem::initData()
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
    connect(m_detailBtn, &DPushButton::clicked, this, &TextNoteItem::onshowDetail);
    connect(m_saveAsAction, &QAction::triggered, this, &TextNoteItem::onSaveText);
    connect(m_delAction, &QAction::triggered, this, &TextNoteItem::onDelItem);
    connect(m_menuBtn, &DPushButton::clicked, this, &TextNoteItem::onMenuPop);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &TextNoteItem::onChangeTheme);
}

void TextNoteItem::onTextChanged()
{
     QTimer::singleShot(0, this, [=]{
        int textEditHeight = m_textEdit->height();
        QTextDocument *document = m_textEdit->document();
        int documentHeight = static_cast<int>(document->size().height());
        if (textEditHeight < documentHeight - 3) {
            m_detailBtn->setVisible(true);
        } else {
            m_detailBtn->setVisible(false);
        }
        emit sigTextEditIsEmpty(m_textNode, document->isEmpty());
     });
}

void TextNoteItem::onshowDetail()
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
        onDelItem();
    } else {
        if (text != m_textNode->noteText) {
            m_textNode->noteText = text;
            emit sigUpdateNote(m_textNode);
        }
    }
    qDebug() << "id:" << m_textNode->noteId << "fouce out";
}

void TextNoteItem::onDelItem()
{
    emit sigDelNote(m_textNode);
}

void TextNoteItem::onSaveText()
{
    DFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("Save as TXT"));
    fileDialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    fileDialog.setFileMode(DFileDialog::Directory);
    fileDialog.setAcceptMode(DFileDialog::AcceptSave);
    fileDialog.setDefaultSuffix("txt");
    fileDialog.setNameFilter("TXT(*.txt)");
    fileDialog.selectFile(tr("TextNote"));
    if (fileDialog.exec() == QDialog::Accepted) {
        QString path = fileDialog.selectedFiles()[0];
        QString fileName = QFileInfo(path).fileName();
        QString filePath = path;
        if (fileName.isEmpty()) {
            DMessageBox::information(this, tr(""), tr("File name cannot be empty"));
        }
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream textStream(&file);
            textStream << m_textNode->noteText;
            file.close();
        }
    }
}

void TextNoteItem::onMenuPop()
{
    QPoint pos = QCursor::pos();
    pos.setX(pos.x() - 44);
    pos.setY(pos.y() + 20);
    m_contextMenu->exec(pos);
}

void TextNoteItem::highSearchText(const QString &searchKey, const QColor &highColor)
{
    if (!searchKey.isEmpty()) {
        if (Utils::highTextEdit(m_textEdit, &m_textEditFormat, searchKey, highColor)) {
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
void TextNoteItem::resizeEvent(QResizeEvent *event)
{
    onTextChanged();
    DWidget::resizeEvent(event);
}
