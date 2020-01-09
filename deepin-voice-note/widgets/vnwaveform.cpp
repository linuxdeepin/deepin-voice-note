#include "widgets/vnwaveform.h"

#include <QMouseEvent>

#include <DLog>

VNWaveform::VNWaveform(QWidget *parent)
    : DSlider(Qt::Horizontal, parent)
{
    slider()->hide();
}

void VNWaveform::paintEvent(QPaintEvent *event)
{

}

void VNWaveform::mouseReleaseEvent(QMouseEvent *event)
{
    //this->blockSignals(false);
    DSlider::mouseReleaseEvent(event);

    //Q_EMIT valueAccpet(value());
}

void VNWaveform::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
            || event->button() == Qt::MiddleButton
            || event->button() == Qt::RightButton) {
        if (orientation() == Qt::Vertical) {
            setValue(minimum() + ((maximum() - minimum()) * (height() - event->y())) / height()) ;
        } else {
            setValue(minimum() + ((maximum() - minimum()) * (event->x())) / (width()));
        }
    }

    //this->blockSignals(true);
}

void VNWaveform::mouseMoveEvent(QMouseEvent *event)
{
    auto valueRange = this->maximum()  - this->minimum();
    auto viewRange = this->width();

    if (0 == viewRange) {
        return;
    }

    auto value = minimum() + ((maximum() - minimum()) * (event->x())) / (width());
    setValue(value);
}

void VNWaveform::enterEvent(QEvent *event)
{
    DSlider::enterEvent(event);
    slider()->show();
}

void VNWaveform::leaveEvent(QEvent *event)
{
    DSlider::leaveEvent(event);
    slider()->hide();
}

void VNWaveform::resizeEvent(QResizeEvent *event)
{
    DSlider::resizeEvent(event);
}
