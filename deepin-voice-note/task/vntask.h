#ifndef VNTASK_H
#define VNTASK_H

#include <QObject>
#include <QRunnable>

class VNTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit VNTask(QObject *parent = nullptr);

signals:

public slots:
};

#endif // VNTASK_H
