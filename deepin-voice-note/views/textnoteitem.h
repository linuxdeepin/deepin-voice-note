#ifndef TEXTNOTEITEM_H
#define TEXTNOTEITEM_H

#include "common/vnoteitem.h"

class TextNoteEdit;

class TextNoteItem : public DetailItemWidget
{
    Q_OBJECT
public:
    explicit TextNoteItem(VNoteBlock *noteBlock, QWidget *parent = nullptr , QRegExp reg = QRegExp());
    VNoteBlock *getNoteBlock();
    QTextCursor getTextCursor();
    void        setTextCursor(const QTextCursor &cursor);
    void        updateData();
    bool        textIsEmpty();
    QRect       getCursorRect();
    void selectText(const QPoint &globalPos,QTextCursor::MoveOperation op);
    void selectText(QTextCursor::MoveOperation op);
    void removeSelectText();
    void selectAllText();
    void clearSelection();
    bool hasSelection();
    QString getAllText();
    QString getSelectText();
    void setFocus();
    bool hasFocus();
signals:
    void        sigCursorHeightChange(QWidget*widget, int height);
    void        sigTextChanged();
    void        sigFocusIn();
    void        sigFocusOut();
    void        sigDelEmpty(); //已经删完了内容还是按删除键
private:
    void         initUi();
    void         initConnection();
    VNoteBlock   *m_noteBlock {nullptr};
    TextNoteEdit *m_textEdit {nullptr};
    QRegExp       m_serchKey ;
    QTextCharFormat m_textCharFormat;

};

#endif // TEXTNOTEITEM_H
