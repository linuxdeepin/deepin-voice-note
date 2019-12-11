#ifndef LOADICONSWORKER_H
#define LOADICONSWORKER_H

#include <QObject>
#include <QRunnable>

class LoadIconsWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit LoadIconsWorker(QObject *parent = nullptr);

    const int DEFAULTICONS_COUNT = 10;
protected:
    virtual void run();

signals:

public slots:
};

#endif // LOADICONSWORKER_H
