#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include "foldertree.h"

#include <DWidget>
#include <DPushButton>

class LeftView : public DWidget
{
    Q_OBJECT
public:
    explicit LeftView(QWidget *parent = nullptr);
    void clearSelection();
    void updateCurFolder();
    void selectDefaultItem();
    void delFolderItem(const QModelIndex &index);
    int  loadNoteFolder();
signals:
    void sigFolderChange(const QModelIndex &index);
    void sigFolderAction(const QModelIndex &index , QAction *action);

public slots:
    void onAddNotepad();
    void onTreeAction(QAction *action);
    void onTreeSelectChange(const QModelIndex &current, const QModelIndex &previous);
private:
    void initFolderTree();
    void initUI();
    void initConnection();

    DPushButton     *m_addNotepadBtn {nullptr};
    FolderTree      *m_folderTree    {nullptr};
    QModelIndex      m_folderLastIndex;
};

#endif // LEFTVIEW_H
