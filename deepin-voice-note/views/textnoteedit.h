#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include<DTextEdit>

DWIDGET_USE_NAMESPACE
struct VNoteItem;
struct VNTextBlock;

class TextNoteEdit : public DTextEdit
{
    Q_OBJECT
public:
    explicit TextNoteEdit(VNoteItem *textNote, VNTextBlock *noteBlock, QWidget *parent = nullptr);
    VNoteItem *getNoteItem();
    VNTextBlock *getNoteBlock();
    void selectText(const QPoint &globalPos, QTextCursor::MoveOperation op);
    void clearSelection();
signals:
    void sigFocusIn();
    void sigFocusOut();
    void sigDelEmpty(); //已经删完了内容还是按删除键
private:
    bool m_menuPop {false};
    VNoteItem       *m_textNode {nullptr};
    VNTextBlock     *m_noteBlock {nullptr};
protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // TEXTEDITITEM_H
