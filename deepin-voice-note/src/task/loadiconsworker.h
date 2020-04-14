#ifndef LOADICONSWORKER_H
#define LOADICONSWORKER_H

#include "vntask.h"

#include <QObject>
#include <QRunnable>

class LoadIconsWorker : public VNTask
{
    Q_OBJECT
public:
    explicit LoadIconsWorker(QObject *parent = nullptr);
    QPixmap greyPix(QPixmap pix);

    const int DEFAULTICONS_COUNT = 10;
protected:
    virtual void run();

signals:

public slots:
};

#endif // LOADICONSWORKER_H
