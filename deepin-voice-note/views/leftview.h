#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include <DTreeView>

DWIDGET_USE_NAMESPACE

class LeftView : public QTreeView
{
    Q_OBJECT
public:
    explicit LeftView(QWidget *parent = nullptr);

signals:

public slots:
};

#endif // LEFTVIEW_H
