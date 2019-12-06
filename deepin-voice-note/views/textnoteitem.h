#ifndef TEXTNOTEITEM_H
#define TEXTNOTEITEM_H

#include "common/datatypedef.h"

#include<QVBoxLayout>
#include<QHBoxLayout>

#include <DWidget>
#include <DLabel>
#include <DFrame>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE
class TextNoteItem : public DWidget
{
    Q_OBJECT
public:
    explicit TextNoteItem(VNoteItem *textNote,QWidget *parent = nullptr);
private:
    void initUI();
    void initData();
    void initConnection();
signals:

public slots:
private:
    DLabel *m_timeLabel {nullptr};
    DFrame *m_bgWidget {nullptr};
    DTextEdit *m_textEdit{nullptr} ;
    VNoteItem *m_textNode {nullptr};
    QVBoxLayout *m_itemLayout {nullptr};
    QHBoxLayout *m_hBoxLayout {nullptr};
};

#endif // TEXTNOTEITEM_H
