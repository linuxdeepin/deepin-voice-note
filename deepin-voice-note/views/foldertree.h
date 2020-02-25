#ifndef FOLDERTREE_H
#define FOLDERTREE_H

#include <QStandardItemModel>

#include <DTreeView>
#include <DLabel>

DWIDGET_USE_NAMESPACE

struct VNOTE_FOLDERS_MAP;
struct VNoteFolder;
class FolderDelegate;

class FolderTree : public DTreeView
{
public:
    enum ItemType {
        NOTEPADROOT, //记事本一级目录
        NOTEPADITEM, //记事本项
        ERRORTYPE
    };
    explicit FolderTree(QWidget *parent = nullptr);
    void addNotepad(VNoteFolder *notepad);
    void itemEditFinsh(const QModelIndex &index,const QString & newValue);
    int addNotepads(VNOTE_FOLDERS_MAP *notepads);

    QStandardItem *getNotepadRoot();
    ItemType getItemType(const QModelIndex &index) const;
    VNoteFolder *getNotepadItemData(const QModelIndex &index) const;

private:
    void initDelegate();
    void initModel();
    void initConnection();
    void initNotepadRoot();
    void setItemType(QStandardItem *pItem ,ItemType type);
    void setItemData(QStandardItem *pItem, void *data);

    QStandardItemModel *m_pDataModel {nullptr};
    FolderDelegate *m_pItemDelegate {nullptr};
};

#endif // FOLDERTREE_H
