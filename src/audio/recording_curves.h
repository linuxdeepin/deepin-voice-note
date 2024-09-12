#ifndef RECORDINGCURVES_H
#define RECORDINGCURVES_H

#include <QObject>
#include <QQuickPaintedItem>
#include <QPainter>

class RecordingCurves : public QQuickPaintedItem
{
    Q_OBJECT
public:
    RecordingCurves(QQuickItem *parent = 0);
    ~RecordingCurves();

    Q_INVOKABLE void updatePoint(const int &index, const QString &direction);
    Q_INVOKABLE void togglePointShow();
    Q_INVOKABLE void updateVolume(const double &gain);
public:
    void paint(QPainter *painter);

private:
    qreal qBinomialCoefficient(int n, int k);
    QPointF bezierPoint(qreal t, const QList<QPointF> &points);

private:
    QList<QPointF> m_pointList;
    QList<QPointF> m_volumeList;
    double m_gain {0.0};
    bool m_isShowPoint {false};
};

#endif // RECORDINGCURVES_H
