#include "vnwaveform.h"

#include <QMouseEvent>
#include <QPen>
#include <QPainter>

#include <DLog>

VNWaveform::VNWaveform(QWidget *parent)
    : DFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
}

void VNWaveform::onAudioBufferProbed(const QAudioBuffer &buffer)
{
    getBufferLevels(buffer, m_audioScaleSamples);
    update();
}

void VNWaveform::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    //Support highquality
    qreal waveWidth = WAVE_WIDTH * devicePixelRatioF();
    qreal waveSpace = WAVE_SPACE * devicePixelRatioF();

    qreal waveOffsetX = WAVE_OFFSET_X;
    qreal waveOffsetY = WAVE_OFFSET_Y;

    const qreal availableHeight = height() - (waveOffsetY *2);

    const QColor startColor(3, 208, 214);
    const QColor endColor(1, 121, 255);

    for (int i = 0; (i < m_audioScaleSamples.size() && i < m_maxShowedSamples); i++) {

        qreal waveHeight = static_cast<int>(m_audioScaleSamples[i] * height());

        //Use default height when waveHeight less or equal to 0
        waveHeight = (waveHeight > 0.0001) ? waveHeight : waveWidth;

        qreal startX = waveOffsetX+i*(waveWidth+waveSpace);
        qreal startY = (availableHeight-waveHeight ) / m_waveStyle;

        QRectF waveRectF(startX, startY, waveWidth, waveHeight);

        QLinearGradient waveGradient(waveRectF.topLeft(), waveRectF.bottomLeft());

        waveGradient.setColorAt(0, startColor);
        waveGradient.setColorAt(1, endColor);

        painter.fillRect(waveRectF, waveGradient);
    }

    painter.restore();
}

void VNWaveform::resizeEvent(QResizeEvent *event)
{
    m_maxShowedSamples = static_cast<int> (
                (event->size().width()-WAVE_OFFSET_X*2)
                / ((WAVE_WIDTH+WAVE_SPACE) * devicePixelRatioF())
                );

    qDebug() << __FUNCTION__ << "m_maxShowedSamples:" << m_maxShowedSamples
             << "width:" << event->size().width();
    DFrame::resizeEvent(event);
}

// returns the audio level for each channel
void VNWaveform::getBufferLevels(const QAudioBuffer &buffer, QVector<qreal> &scaleSamples)
{
    QVector<qreal> values;

    if (!buffer.format().isValid() || buffer.format().byteOrder() != QAudioFormat::LittleEndian)
        return;

    if (buffer.format().codec() != "audio/pcm")
        return;

    int channelCount = buffer.format().channelCount();

    scaleSamples.reserve(buffer.frameCount());
    scaleSamples.fill(0, buffer.frameCount());

    qreal peak_value = VNWaveform::getPeakValue(buffer.format());

    switch (buffer.format().sampleType()) {
    case QAudioFormat::Unknown:
    case QAudioFormat::UnSignedInt:
        if (buffer.format().sampleSize() == 32)
            VNWaveform::getBufferLevels(buffer.constData<quint32>(), buffer.frameCount(), channelCount, peak_value, scaleSamples);
        if (buffer.format().sampleSize() == 16)
            VNWaveform::getBufferLevels(buffer.constData<quint16>(), buffer.frameCount(), channelCount, peak_value, scaleSamples);
        if (buffer.format().sampleSize() == 8)
            VNWaveform::getBufferLevels(buffer.constData<quint8>(), buffer.frameCount(), channelCount, peak_value, scaleSamples);
        break;
    case QAudioFormat::Float:
        if (buffer.format().sampleSize() == 32) {
            VNWaveform::getBufferLevels(buffer.constData<float>(), buffer.frameCount(), channelCount, peak_value, scaleSamples);
        }
        break;
    case QAudioFormat::SignedInt:
        if (buffer.format().sampleSize() == 32)
            VNWaveform::getBufferLevels(buffer.constData<qint32>(), buffer.frameCount(), channelCount, peak_value, scaleSamples);
        if (buffer.format().sampleSize() == 16)
            VNWaveform::getBufferLevels(buffer.constData<qint16>(), buffer.frameCount(), channelCount, peak_value, scaleSamples);
        if (buffer.format().sampleSize() == 8)
            VNWaveform::getBufferLevels(buffer.constData<qint8>(), buffer.frameCount(), channelCount, peak_value, scaleSamples);
        break;
    }
}

template <class T>
void VNWaveform::getBufferLevels(const T *buffer, int frames, int channels, qreal peakValue, QVector<qreal> &samples)
{

    for (int i = 0; i < frames; ++i) {
        qreal averageValue = 0;

        for (int j = 0; j < channels; ++j) {
            averageValue += qAbs(qreal(buffer[i * channels + j]));
        }

        averageValue = (averageValue) / channels;

        samples[i] = averageValue / peakValue;
    }
}

// This function returns the maximum possible sample value for a given audio format
qreal VNWaveform::getPeakValue(const QAudioFormat &format)
{
    // Note: Only the most common sample formats are supported
    if (!format.isValid())
        return qreal(0);

    if (format.codec() != "audio/pcm")
        return qreal(0);

    switch (format.sampleType()) {
    case QAudioFormat::Unknown:
        break;
    case QAudioFormat::Float:
        if (format.sampleSize() != 32) // other sample formats are not supported
            return qreal(0);
        return qreal(1.00003);
    case QAudioFormat::SignedInt:
        if (format.sampleSize() == 32)
            return qreal(INT_MAX);
        if (format.sampleSize() == 16)
            return qreal(SHRT_MAX);
        if (format.sampleSize() == 8)
            return qreal(CHAR_MAX);
        break;
    case QAudioFormat::UnSignedInt:
        if (format.sampleSize() == 32)
            return qreal(UINT_MAX);
        if (format.sampleSize() == 16)
            return qreal(USHRT_MAX);
        if (format.sampleSize() == 8)
            return qreal(UCHAR_MAX);
        break;
    }

    return qreal(0);
}
