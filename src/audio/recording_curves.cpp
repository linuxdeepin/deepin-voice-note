#include "recording_curves.h"

#include <QPainter>
#include <cmath>
#include <QtMath>
#include <QLinearGradient>
#include <QDebug>

RecordingCurves::RecordingCurves(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_timer(new QTimer(this))
{
    qInfo() << "RecordingCurves constructor called";
    m_timer->setInterval(1000 / 45);
    connect(m_timer, &QTimer::timeout, this, &RecordingCurves::updateCurves);
    qInfo() << "RecordingCurves constructor finished";
}

RecordingCurves::~RecordingCurves()
{
    // qInfo() << "RecordingCurves destructor called";
}

void RecordingCurves::updateVolume(const double &gain)
{
    // qInfo() << "updateVolume called with gain:" << gain;
    m_gain = gain;
    update();
    // qInfo() << "updateVolume finished";
}

void RecordingCurves::startRecording()
{
    qInfo() << "startRecording called";
    m_timer->start();
    qInfo() << "startRecording finished";
}

void RecordingCurves::stopRecording()
{
    qInfo() << "stopRecording called";
    m_timer->stop();
    m_phase = 0.0;
    m_gain = 0.0;  // 添加这一行，重置增益为0
    update();
    qInfo() << "stopRecording finished";
}

void RecordingCurves::pauseRecording()
{
    qInfo() << "pauseRecording called, timer active:" << m_timer->isActive();
    if (m_timer->isActive()) {
        m_gain = 0.0;
        m_timer->stop();
        qInfo() << "Recording paused";
    } else {
        m_timer->start();
        qInfo() << "Recording resumed";
    }
    qInfo() << "pauseRecording finished";
}

void RecordingCurves::updateCurves()
{
    // qInfo() << "updateCurves called";
    m_phase += M_PI;
    if (m_phase > 170*M_PI)
        m_phase = 0;
    update();
    // qInfo() << "updateCurves finished";
}

void RecordingCurves::paint(QPainter *painter)
{
    // qInfo() << "paint called";
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
        double a = std::pow(x - width / 2, 2);
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

#ifdef __mips64
    // MIPS64 架构：避免直接使用 QLinearGradient 构造 QBrush
    // 分步创建以避免内存对齐问题
    QBrush gradientBrush;
    gradientBrush = QBrush(gradient);
    QPen gradientPen(gradientBrush, 1);
    painter->setPen(gradientPen);
#else
    // 非 MIPS64 架构使用原始方法
    painter->setPen(QPen(QBrush(gradient), 1));
#endif
    painter->drawPolyline(points.constData(), points.size());
    // qInfo() << "paint finished";
}
