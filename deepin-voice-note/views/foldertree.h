#ifndef FOLDERTREE_H
#define FOLDERTREE_H

#include "folderdelegate.h"

#include <QStandardItemModel>

#include <DTreeView>

DWIDGET_USE_NAMESPACE

class FolderTree : public DTreeView
{
public:
    enum TreeRootType {Notepad};
    explicit FolderTree(QWidget *parent = nullptr);
    void addNotepad();
    void initNotepadRoot();
    QModelIndex getNotepadRoot();
    FolderDelegate::ItemType getItemType(const QModelIndex &index) const;

private:
    void initDelegate();
    void initModel();

    QStandardItemModel *m_pDataModel {nullptr};
    FolderDelegate *m_pItemDelegate {nullptr};
};

#endif // FOLDERTREE_H
