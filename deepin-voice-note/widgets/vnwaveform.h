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
};

#endif // VNWAVEFORM_H
