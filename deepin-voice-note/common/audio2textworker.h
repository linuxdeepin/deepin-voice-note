#ifndef AUDIO2TEXTWORKER_H
#define AUDIO2TEXTWORKER_H

#include <QObject>
#include <QRunnable>

class VNoteAudioDeviceWatcher;

class Audio2TextWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit Audio2TextWorker(QObject *parent = nullptr);

protected:
    virtual void run();

signals:

public slots:
protected:
};

#endif // AUDIO2TEXTWORKER_H
