#include "recording_curves.h"

#include <QLinearGradient>

RecordingCurves::RecordingCurves(QQuickItem *parent)
    :QQuickPaintedItem(parent)
{
    m_pointList << QPointF(0,12) << QPointF(13,18) << QPointF(25,20) << QPointF(36,7) << QPointF(57,-4) << QPointF(75,-4) << QPointF(90,12)
                << QPointF(105,28) << QPointF(121,28) << QPointF(139,16) << QPointF(154,3) << QPointF(167,5) << QPointF(180,12);
    foreach (QPointF point, m_pointList) {
        m_volumeList.append(QPointF(point.x(), 12));
    }
}

RecordingCurves::~RecordingCurves()
{

}

void RecordingCurves::updatePoint(const int &index, const QString &direction)
{
    if (index < 0 || index > 12)
        return;
    QPointF point = m_pointList.at(index);
    if (direction == "up")
        point.setY(point.y()-1);
    else if (direction == "down")
        point.setY(point.y()+1);
    else if (direction == "left")
        point.setX(point.x()-1);
    else if (direction == "right")
        point.setX(point.x()+1);

    m_pointList.removeAt(index);
    m_pointList.insert(index, point);
    update();
}

void RecordingCurves::togglePointShow()
{
    m_isShowPoint = !m_isShowPoint;
    update();
}

void RecordingCurves::updateVolume(const double &gain)
{
    m_gain = gain;
    m_volumeList.clear();
    for (int i = 0; i < m_pointList.size(); ++i) {
        QPointF p = m_pointList[i];
        p.setY(12 + ((p.y() - 12) * m_gain));
        m_volumeList.append(p);
    }
    update();
}

// 计算组合数 C(n, k)
qreal RecordingCurves::qBinomialCoefficient(int n, int k) {
    qreal result = 1;
    for (int i = 0; i < k; ++i) {
        result *= (n - i);
        result /= (i + 1);
    }
    return result;
}

QPointF RecordingCurves::bezierPoint(qreal t, const QList<QPointF> &points) {
    int n = points.size() - 1;
    QPointF point(0, 0);
    for (int i = 0; i <= n; ++i) {
        qreal coeff = qPow(1 - t, n - i) * qPow(t, i) * qBinomialCoefficient(n, i);
        point += points[i] * coeff;
    }
    return point;
}

void RecordingCurves::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制控制点及其连线
    if (m_isShowPoint) {
        painter->setPen(Qt::red);
        for (int i = 0; i < m_pointList.size(); ++i) {
            painter->drawEllipse(m_pointList[i], 5, 5);
            if (i > 0) {
                painter->drawLine(m_pointList[i - 1], m_pointList[i]);
            }
        }
    }

    auto rect = boundingRect();
    QLinearGradient gradient(rect.topLeft(), rect.topRight());
    gradient.setColorAt(0, QColor(253, 100, 100, 206));
    gradient.setColorAt(0.51, QColor(255, 0, 0));
    gradient.setColorAt(1, QColor(254, 143, 143));

    int steps = 100;
    QPointF lastPoint(0, 0);
    QPointF lastBackPoint(0, 0);
    QList<QLineF> lines;
    QList<QLineF> backLines;
    for (int i = 0; i <= steps; ++i) {
        qreal t = i / static_cast<qreal>(steps);
        QPointF point = bezierPoint(t, m_volumeList);
        QPointF backPoint(point.x(), 24-point.y());
        if (i > 0) {
            lines.append(QLineF(lastPoint, point));
            backLines.append(QLineF(lastBackPoint, backPoint));
        }
        lastPoint = point;
        lastBackPoint = backPoint;
    }

    painter->setPen(QColor(255, 200, 200));
    painter->drawLines(backLines);

    QPen shadows;
    shadows.setColor(QColor(255, 1, 1, 15));
    shadows.setWidth(4);
    painter->setPen(shadows);
    painter->drawLines(lines);

    painter->setPen(QPen(QBrush(gradient), 1));
    painter->drawLines(lines);
}
