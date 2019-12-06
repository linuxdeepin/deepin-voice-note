#ifndef RIGHTNOTELIST_H
#define RIGHTNOTELIST_H
#include "common/datatypedef.h"

#include <DListWidget>
#include <QSharedPointer>
#include <QList>

DWIDGET_USE_NAMESPACE

class RightNoteList : public DListWidget
{
    Q_OBJECT
public:
    explicit RightNoteList(qint64 folderId,QWidget *parent = nullptr);

    qint64 getFolderId();


signals:

public slots:
    void loadNoteItem();
    void addTextNodeItem();
    void addNodeItem(VNoteItem *item);
private:
    void initUI();
    void initConnection();
    qint64 getNewNoteId();
    void adjustWidgetItemWidth();
//protected:
//    void paintEvent(QPaintEvent *event);
      void resizeEvent(QResizeEvent * event);
//    void wheelEvent(QWheelEvent *e);
private:
    qint64 m_folderId {-1};
    VNOTE_ITEMS_MAP *m_mapNoteData {nullptr};
    QList<QSharedPointer<VNoteItem>> data;
};

#endif // RIGHTNOTELIST_H
