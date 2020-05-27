#ifndef MIDDLEVIEWSORTFILTER_H
#define MIDDLEVIEWSORTFILTER_H

#include <QSortFilterProxyModel>

class MiddleViewSortFilter : public QSortFilterProxyModel
{
public:
    MiddleViewSortFilter(QObject *parent=nullptr);

    enum sortFeild {
        title,
        createTime,
        modifyTime,
    };

    void sortView(
            sortFeild feild=modifyTime,
            int column=0,
            Qt::SortOrder order=Qt::DescendingOrder);

protected:
    virtual bool lessThan(
            const QModelIndex &source_left,
            const QModelIndex &source_right ) const override;

    sortFeild m_sortFeild {modifyTime};
};

#endif // MIDDLEVIEWSORTFILTER_H
