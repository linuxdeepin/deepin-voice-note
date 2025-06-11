#include "recording_curves.h"

#include <QLinearGradient>

RecordingCurves::RecordingCurves(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(1000 / 45);
    connect(m_timer, &QTimer::timeout, this, &RecordingCurves::updateCurves);
}

RecordingCurves::~RecordingCurves()
{

}

void RecordingCurves::updateVolume(const double &gain)
{
    m_gain = gain;
    update();
}

void RecordingCurves::startRecording()
{
    m_timer->start();
}

void RecordingCurves::stopRecording()
{
    m_timer->stop();
    m_phase = 0.0;
    m_gain = 0.0;  // 添加这一行，重置增益为0
    update();
}

void RecordingCurves::pauseRecording()
{
    if (m_timer->isActive()) {
        m_gain = 0.0;
        m_timer->stop();
    } else {
        m_timer->start();
    }
}

void RecordingCurves::updateCurves()
{
    m_phase += M_PI;
    if (m_phase > 170*M_PI)
        m_phase = 0;
    update();
}

void RecordingCurves::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);

    auto rect = boundingRect();
    QLinearGradient gradient(rect.topLeft(), rect.topRight());
    gradient.setColorAt(0, QColor(253, 100, 100, 206));
    gradient.setColorAt(0.51, QColor(255, 0, 0));
    gradient.setColorAt(1, QColor(254, 143, 143));

    double width = this->width();
    double height = this->height();

    QVector<QPointF> points;
    QVector<QPointF> backPoints;

    for (double x = 0; x <= width; x += 1.0) {
        double ySin = std::sin((2 * (x-m_phase)) / width * 2 * M_PI);
        double a = pow(x - width / 2, 2);
        double b = -4 / (width * width);
        double yPara = a * b + 1;
        double pointY = ySin * yPara * height * m_gain / 2.0;
        double mappedY = (height/2) - pointY;
        double mappedYNeg = (height/2) - (-pointY);
        points.append(QPointF(x, mappedY));
        backPoints.append(QPointF(x, mappedYNeg));
    }

    painter->setPen(QColor(255, 200, 200));
    painter->drawPolyline(backPoints.constData(), backPoints.size());

    QPen shadows;
    shadows.setColor(QColor(255, 1, 1, 15));
    shadows.setWidth(4);
    painter->setPen(shadows);
    painter->drawPolyline(points.constData(), points.size());

    painter->setPen(QPen(QBrush(gradient), 1));
    painter->drawPolyline(points.constData(), points.size());
}
