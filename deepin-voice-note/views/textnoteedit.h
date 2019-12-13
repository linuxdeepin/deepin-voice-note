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
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
};

#endif // TEXTNOTEEDIT_H
