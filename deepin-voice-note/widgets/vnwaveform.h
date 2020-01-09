#ifndef VNWAVEFORM_H
#define VNWAVEFORM_H

#include <DSlider>

DWIDGET_USE_NAMESPACE

class VNWaveform : public DSlider
{
    Q_OBJECT
public:
    explicit VNWaveform(QWidget *parent = nullptr);

signals:

public slots:
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // VNWAVEFORM_H
