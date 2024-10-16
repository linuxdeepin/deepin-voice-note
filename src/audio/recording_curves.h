#ifndef RECORDINGCURVES_H
#define RECORDINGCURVES_H

#include <QObject>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QTimer>

class RecordingCurves : public QQuickPaintedItem
{
    Q_OBJECT
public:
    RecordingCurves(QQuickItem *parent = 0);
    ~RecordingCurves();

    Q_INVOKABLE void updateVolume(const double &gain);
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void pauseRecording();
public:
    void paint(QPainter *painter);

private slots:
    void updateCurves();

private:
    QList<QPointF> m_pointList;
    QList<QPointF> m_volumeList;
    double m_gain {0.0};
    double m_phase;
    QTimer *m_timer;
};

#endif // RECORDINGCURVES_H
