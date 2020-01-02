#ifndef TEXTNOTEITEM_H
#define TEXTNOTEITEM_H

#include "common/datatypedef.h"
#include "textnoteedit.h"
#include "myrecodebuttons.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <DFrame>
#include <DLabel>
#include <DMenu>
#include <DPushButton>
#include <DTextEdit>
#include <DWidget>
#include <DIconButton>

DWIDGET_USE_NAMESPACE
class TextNoteItem : public DWidget
{
    Q_OBJECT
public:
    explicit TextNoteItem(VNoteItem *textNote, QWidget *parent = nullptr);
    void highSearchText(const QRegExp &searchKey, const QColor &highColor);
    VNoteItem *getNoteItem();
    void changeToEdit();
    void updateData();
signals:
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void sigDelNote(VNoteItem *textNode);
    void sigUpdateNote(VNoteItem *textNode);
    void sigTextEditIsEmpty(VNoteItem *textNode, bool empty);

public slots:
    void onTextChanged();
    void onshowDetail();
    void onEditFocusIn();
    void onEditFocusOut();
    void onSaveText();
    void onDelItem();
    void onMenuPop();
    void onChangeTheme();

private:
    void initUI();
    void initConnection();
    void adjustDocMargin();
protected:
    void resizeEvent(QResizeEvent *event);
private:
    DLabel *m_timeLabel {nullptr};

    TextNoteEdit *m_textEdit {nullptr};
    VNoteItem *m_textNode {nullptr};
    MyRecodeButtons *m_detailBtn {nullptr};
    MyRecodeButtons *m_menuBtn {nullptr};

    DMenu *m_contextMenu {nullptr};
    QAction *m_saveAsAction {nullptr};
    QAction *m_delAction{nullptr};
    QRegExp m_searchKey;
    QTextCharFormat m_textEditFormat;
};

#endif  // TEXTNOTEITEM_H
