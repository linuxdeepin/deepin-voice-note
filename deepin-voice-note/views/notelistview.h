#ifndef NOTELISTVIEW_H
#define NOTELISTVIEW_H

#include "noteitemdelegate.h"

#include <DListView>

DWIDGET_USE_NAMESPACE

struct VNOTE_ITEMS_MAP;
struct VNoteItem;
struct VNoteFolder;

class NoteListView :public DListView
{
public:
    NoteListView(QWidget *parent = nullptr);
    const QStandardItem *addNoteItem(VNoteItem* data);
    void removeNoteItem(VNoteItem* data);
    void removeCurItem();
    void clearAllItems();
    void initNoteData(VNOTE_ITEMS_MAP *data,const QRegExp &searchKey);
    void itemEditFinsh(const QModelIndex &index,const QString & newValue);

    VNoteItem*  getItemData(const QModelIndex &index) const;
    VNoteFolder *getFolderData(qint64 id);
    QModelIndex getItemIndex(const VNoteItem* data) const;
private:
    void initConnection();
    void initDelegate();
    void initModel();

    QStandardItemModel *m_pDataModel {nullptr};
    NoteItemDelegate *m_pItemDelegate {nullptr};

};

#endif // NOTELISTVIEW_H
