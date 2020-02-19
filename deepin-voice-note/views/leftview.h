#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include "foldertree.h"

#include <DWidget>
#include <DPushButton>
DWIDGET_USE_NAMESPACE

class LeftView : public DWidget
{
    Q_OBJECT
public:
    explicit LeftView(QWidget *parent = nullptr);
    void clearTreeSelection();

signals:
    void sigNotepadAdd();
    void sigModeIndexChange(const QModelIndex &index, int type);

public slots:
    void onAddNotepad();
    void onTreeItemClicked(const QModelIndex &index);
private:
    void initFolderTree();
    void initUI();
    void initConnection();

    DPushButton     *m_addNotepadBtn {nullptr};
    FolderTree *m_folderTree {nullptr};
    QModelIndex m_folderLastIndex;
};

#endif // LEFTVIEW_H
