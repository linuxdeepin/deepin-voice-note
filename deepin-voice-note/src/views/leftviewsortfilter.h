#ifndef LEFTVIEWSORTFILTER_H
#define LEFTVIEWSORTFILTER_H
#include <QSortFilterProxyModel>

class LeftViewSortFilter : public QSortFilterProxyModel
{
public:
    LeftViewSortFilter(QObject *parent=nullptr);

protected:
    virtual bool lessThan(
            const QModelIndex &source_left,
            const QModelIndex &source_right ) const override;

};

#endif // LEFTVIEWSORTFILTER_H
