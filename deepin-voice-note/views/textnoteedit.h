#ifndef TEXTNOTEEDIT_H
#define TEXTNOTEEDIT_H

#include <DTextEdit>

DWIDGET_USE_NAMESPACE
class TextNoteEdit : public DTextEdit
{
    Q_OBJECT
public:
    explicit TextNoteEdit(QWidget *parent = nullptr);
    void setLineHeight(int height);
    void setDocMargin(const QMargins &margins);
    void setPlainText(const QString &text);
signals:
    void sigFocusIn();
    void sigFocusOut();
    void sigDocumentSizeChange();
public slots:
     void onAdjustDocument();
     void onDocumentSizeChanged();
private:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void insertFromMimeData(const QMimeData *source) override;
private:
    bool m_menuPop {false};
    QMargins m_docMargins;
    int      m_lineHeight {0};
};

#endif // TEXTNOTEEDIT_H
