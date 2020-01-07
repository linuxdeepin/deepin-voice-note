#include "dialog/vnoteiconbutton.h"

#include <QImageReader>

VNoteIconButton::VNoteIconButton(QString normal,QString hover, QString press,QWidget *parent)
    : DIconButton(parent)
{
    m_icons[Normal] = normal;
    m_icons[Hover] = hover;
    m_icons[Press] = press;

    setIcon(loadPixmap(m_icons[m_state]));
}

void VNoteIconButton::enterEvent(QEvent *event)
{
    Q_UNUSED(event);

    m_state = Hover;

    setIcon(loadPixmap(m_icons[m_state]));
}

void VNoteIconButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    m_state = Normal;

    setIcon(loadPixmap(m_icons[m_state]));
}

void VNoteIconButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    m_state = Press;

    setIcon(loadPixmap(m_icons[m_state]));

    event->accept();
}

void VNoteIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();

    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (rect().contains(event->pos())) {

        m_state = Hover;

        setIcon(loadPixmap(m_icons[m_state]));

        Q_EMIT clicked();
    }
}

void VNoteIconButton::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if (m_state != Hover) {
        m_state = Hover;

        setIcon(loadPixmap(m_icons[m_state]));
    }
}

QPixmap VNoteIconButton::loadPixmap(const QString &path)
{
    qreal ratio = 1.0;

    const qreal devicePixelRatio = devicePixelRatioF();

    QPixmap pixmap;

    if (!qFuzzyCompare(ratio, devicePixelRatio)) {
        QImageReader reader;
        reader.setFileName(qt_findAtNxFile(path, devicePixelRatio, &ratio));
        if (reader.canRead()) {
            reader.setScaledSize(reader.size() * (devicePixelRatio / ratio));
            pixmap = QPixmap::fromImage(reader.read());
            pixmap.setDevicePixelRatio(devicePixelRatio);
        }
    } else {
        pixmap.load(path);
    }

    return pixmap;
}
