#ifndef TEXTNOTEITEM_H
#define TEXTNOTEITEM_H

#include "common/vnoteitem.h"

class TextNoteEdit;

class TextNoteItem : public DetailItemWidget
{
    Q_OBJECT
public:
    explicit TextNoteItem(VNoteBlock *noteBlock, QWidget *parent = nullptr);
    VNoteBlock *getNoteBlock();
    QTextCursor getTextCursor();
    void        setTextCursor(const QTextCursor &cursor);
    void        updateSearchKey(QString searchKey);
    bool        textIsEmpty();
    QRect       getCursorRect();
    void selectText(const QPoint &globalPos, QTextCursor::MoveOperation op);
    void selectText(QTextCursor::MoveOperation op);
    void removeSelectText();
    void selectAllText();
    void clearSelection();
    bool hasSelection();
    QTextDocumentFragment getSelectFragment();
    QTextDocument* getTextDocument();

    void pasteText();
    void setFocus();
    bool hasFocus();
    bool isSelectAll();

signals:
    void        sigCursorHeightChange(QWidget *widget, int height);
    void        sigTextChanged();
    void        sigFocusIn();
    void        sigFocusOut();
    void        sigSelectionChanged();
public slots:
    void        onTextChange();

private:
    void         initUi();
    void         initConnection();
    bool m_isSearching {false};
    bool m_selectAll   {false};
    bool m_textDocumentUndo  {false};
    int           m_searchCount {0};
    VNoteBlock   *m_noteBlock {nullptr};
    TextNoteEdit *m_textEdit {nullptr};
    QString       m_serchKey ;

};

#endif // TEXTNOTEITEM_H
