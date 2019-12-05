#ifndef LEFTVIEWSORTFILTERMODEL_H
#define LEFTVIEWSORTFILTERMODEL_H

#include <QDateTime>
#include <QSortFilterProxyModel>

class LeftViewSortFilterModel : public QSortFilterProxyModel
{
public:
    enum OperaType { none, folderName, createTime, modifyTime };
    LeftViewSortFilterModel(QObject *parent = nullptr);
    void sortView(OperaType Type = none, int column = 0,
                  Qt::SortOrder order = Qt::AscendingOrder);
    void setCreateTimeFilter(const QDateTime &begin, const QDateTime &end);
    void setModifyTimeFilter(const QDateTime &begin, const QDateTime &end);
    void setFolderNameFilter(QString key = "");
    void clearFilter();

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    OperaType m_sortType {none};
    OperaType m_filterType {none};
    QDateTime m_beginTime;
    QDateTime m_endTime;
};

#endif // LEFTVIEWSORTFILTERMODEL_H
