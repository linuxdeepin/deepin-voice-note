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
    void highSearchText(const QString &searchKey,const QColor &highColor);
    VNoteItem *getNoteItem();
    void changeToEdit();
signals:
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit,const QString &searchKey);
    void sigDelNote(VNoteItem *textNode);
    void sigUpdateNote(VNoteItem *textNode);
    void sigTextEditIsEmpty(VNoteItem *textNode,bool empty);

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
    void initData();
    void initConnection();
protected:
    void resizeEvent(QResizeEvent * event);
private:
    DLabel *m_timeLabel {nullptr};
    DFrame *m_bgWidget {nullptr};

    TextNoteEdit *m_textEdit {nullptr};
    VNoteItem *m_textNode {nullptr};
    MyRecodeButtons *m_detailBtn {nullptr};
    MyRecodeButtons *m_menuBtn {nullptr};

    DMenu *m_contextMenu {nullptr};
    QAction *m_saveAsAction {nullptr};
    QAction *m_delAction{nullptr};
    QString  m_searchKey {""};
    QTextCharFormat m_textEditFormat;
};

#endif  // TEXTNOTEITEM_H
