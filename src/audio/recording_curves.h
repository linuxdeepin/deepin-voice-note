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

    // 查询录音定时器是否处于激活状态（供测试与外部状态查询使用）
    bool isRecordingActive() const;
public:
    void paint(QPainter *painter);

private slots:
    void updateCurves();

private:
    QList<QPointF> m_pointList;
    QList<QPointF> m_volumeList;
    double m_gain {0.0};
    double m_phase;
    // 录音流程是否进行中（startRecording 置位，stopRecording 复位）
    // pauseRecording 仅在该标志为 true 时生效，避免非录制态误启动定时器
    bool m_recordingActive {false};
    QTimer *m_timer;
};

#endif // RECORDINGCURVES_H
