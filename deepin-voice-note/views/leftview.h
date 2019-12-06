#ifndef LEFTVIEW_H
#define LEFTVIEW_H
#include "leftviewdelegate.h"
#include "leftviewsortfiltermodel.h"

#include "common/vnoteforlder.h"

#include <DListView>
#include <DMenu>
#include <DPushButton>

DWIDGET_USE_NAMESPACE
class LeftView : public DListView
{
    Q_OBJECT
public:
    LeftView(QWidget *parent = nullptr);
    qint64 getFolderId(const QModelIndex &index);
    void setFoderUpdateTime(const QDateTime &time, int id, bool sort = false);
    void sortView(LeftViewSortFilterModel::OperaType Type = LeftViewSortFilterModel::none,
                  int column = 0, Qt::SortOrder order = Qt::AscendingOrder);
    void setCreateTimeFilter(const QDateTime &begin, const QDateTime &end);
    void setUpdateTimeFilter(const QDateTime &begin, const QDateTime &end);
    void setFolderNameFilter(QString key = "");
    void clearFilter();
    void loadNoteFolder();

public slots:
    void handleAddFolder();
    void handleRenameItem(bool);
    void handleDeleteItem(bool);
    void handleFolderRename(const VNoteFolder *data);

signals:
    void sigFolderAdd(qint64 id);
    void sigFolderDel(qint64 id);
    void sigFolderUpdate(qint64 id);

private:
    void initMenuAction();
    void initConnection();
    void initDelegate();
    void initModel();
    void initFolderData();

    qint64 getNewFolderId();
    QString getNewFolderIconPath();
protected:
    void mousePressEvent(QMouseEvent *event);

private:
    DMenu *m_contextMenu;
    QAction *m_renameAction;
    QAction *m_delAction;
    VNOTE_FOLDERS_MAP* m_mapFolderData {nullptr};

    QStandardItemModel *m_pDataModel;
    LeftViewSortFilterModel *m_pSortFilterModel;
    LeftViewDelegate *m_pItemDelegate;
};

#endif // LEFTVIEW_H
