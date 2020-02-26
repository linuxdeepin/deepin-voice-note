#ifndef FOLDERTREE_H
#define FOLDERTREE_H

#include <QStandardItemModel>

#include <DTreeView>
DWIDGET_USE_NAMESPACE

class FolderDelegate;
class FolderTree : public DTreeView
{
    Q_OBJECT
public:
    explicit FolderTree(QWidget *parent = nullptr);
    QStandardItem *getNotepadRoot();
signals:
    void treeAction(QAction *action);
protected:
    void mousePressEvent(QMouseEvent *event);
private:
    void initDelegate();
    void initModel();
    void initConnection();
    void initNotepadRoot();
    QStandardItemModel *m_pDataModel {nullptr};
    FolderDelegate   *m_pItemDelegate {nullptr};
    DMenu            *m_notepadMenu {nullptr};
};

#endif // FOLDERTREE_H
