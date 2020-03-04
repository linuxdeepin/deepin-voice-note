#ifndef VNWAVEFORM_H
#define VNWAVEFORM_H

#include <QAudioBuffer>

#include <DFrame>

DWIDGET_USE_NAMESPACE

class VNWaveform : public DFrame
{
    Q_OBJECT
public:
    explicit VNWaveform(QWidget *parent = nullptr);
    const int WAVE_WIDTH = 2;
    void setSliderVisable(bool isVisable);
signals:

public slots:
    void onAudioBufferProbed(const QAudioBuffer &buffer);
protected:
    static qreal getPeakValue(const QAudioFormat &format);
    static void getBufferLevels(const QAudioBuffer &buffer, QVector<qreal> &scaleSamples);

    template <class T>
    static void getBufferLevels(const T *buffer,
                                int frames,
                                int channels,
                                qreal peakValue,
                                QVector<qreal> &samples);

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
protected:
    QVector<qreal> m_audioScaleSamples;
    int            m_maxShowedSamples;
};

#endif // VNWAVEFORM_H
