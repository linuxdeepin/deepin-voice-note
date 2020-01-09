#ifndef TEXTNOTEEDIT_H
#define TEXTNOTEEDIT_H

#include <DTextEdit>

DWIDGET_USE_NAMESPACE
class TextNoteEdit : public DTextEdit
{
    Q_OBJECT
public:
    explicit TextNoteEdit(QWidget *parent = nullptr);

signals:
    void sigFocusIn();
    void sigFocusOut();
public slots:

private:
    void leaveEvent(QEvent *event) override;
    void enterEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;//3699
private:
    bool m_menuPop {false};
};

#endif // TEXTNOTEEDIT_H
