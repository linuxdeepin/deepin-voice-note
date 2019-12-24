#ifndef RIGHTNOTELIST_H
#define RIGHTNOTELIST_H
#include "common/datatypedef.h"

#include <QSharedPointer>
#include <QList>
#include <DListWidget>
#include <DPushButton>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE

class RightNoteList : public DListWidget
{
    Q_OBJECT
public:
    explicit RightNoteList(QWidget *parent = nullptr);
    void initNoteItem(qint64 folderid,VNOTE_ITEMS_MAP *mapNoteData,QString serachKey = "");
    void addNodeItem(VNoteItem *item,QString key = "");
    void delNodeItem(VNoteItem *item);
    int  getListHeight();
    qint64 getFolderId();

signals:
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit,const QString &searchKey);
    void sigDelNote(VNoteItem *textNode);
    void sigUpdateNote(VNoteItem *textNode);
    void sigTextEditIsEmpty(VNoteItem *textNode,bool empty);

private:
    void initUI();
    void initConnection();
    void adjustWidgetItemWidth();

protected:
      void resizeEvent(QResizeEvent * event);
      int m_listHeight {0};
      qint64 m_forlderId {-1};
};

#endif // RIGHTNOTELIST_H
