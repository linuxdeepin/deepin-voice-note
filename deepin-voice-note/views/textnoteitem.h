#ifndef TEXTNOTEITEM_H
#define TEXTNOTEITEM_H

#include "common/vnoteitem.h"
#include "textnoteedit.h"
#include "myrecodebuttons.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <DFrame>
#include <DLabel>
#include <DPushButton>
#include <DTextEdit>
#include <DWidget>
#include <DIconButton>

DWIDGET_USE_NAMESPACE
class TextNoteItem : public VNoteItemWidget
{
    Q_OBJECT
public:
    explicit TextNoteItem(VNoteItem *textNote, QWidget *parent = nullptr);
    void highSearchText(const QRegExp &searchKey, const QColor &highColor);
    VNoteItem *getNoteItem() override;
    void changeToEdit();
    void updateData();
signals:
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void sigDelNote(VNoteItem *textNode);
    void sigMenuPopup(VNoteItem *textNode);
    void sigUpdateNote(VNoteItem *textNode);
    void sigTextEditIsEmpty(VNoteItem *textNode, bool empty);

public slots:
    void onTextChanged();
    void onShowDetail();
    void onEditFocusIn();
    void onEditFocusOut();
    void onShowMenu();
    void onChangeTheme();
private:
    void initUI();
    void initConnection();
    void adjustTextEdit();
    void updateDetilBtn();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void enterEvent(QEvent *event) override;
private:
    DLabel *m_timeLabel {nullptr};

    TextNoteEdit *m_textEdit {nullptr};
    VNoteItem *m_textNode {nullptr};
    MyRecodeButtons *m_detailBtn {nullptr};
    MyRecodeButtons *m_menuBtn {nullptr};
    QRegExp m_searchKey;
    QTextCharFormat m_textEditFormat;
};

#endif  // TEXTNOTEITEM_H
