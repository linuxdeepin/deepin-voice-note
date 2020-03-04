#include "vnwaveform.h"

#include <QMouseEvent>
#include <QPen>
#include <QPainter>

#include <DLog>

VNWaveform::VNWaveform(QWidget *parent)
    : DFrame(parent)
{
    ;
}

void VNWaveform::onAudioBufferProbed(const QAudioBuffer &buffer)
{
    getBufferLevels(buffer, m_audioScaleSamples);
    update();
}

void VNWaveform::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    //painter.setRenderHint(QPainter::Antialiasing, true);
    // 设置画笔颜色、宽度
    //painter.setPen(QPen(QColor(3, 208, 214)/*Qt::blue*/, 0));
    // 设置画刷颜色
    //painter.setBrush(QColor(3, 208, 214)/*Qt::blue*/);
    static float xStep = 4.0f;//278 / 3 = 92
    //static double hScale = height() / 10.0f;
    float dr = 3.0f / m_maxShowedSamples;
    float dg = 87.0f / m_maxShowedSamples;
    float db = 41.0f / m_maxShowedSamples;

    for(int i = 0; (i < m_audioScaleSamples.size() && i < m_maxShowedSamples); i++)
    {

        int h = static_cast<int>(m_audioScaleSamples[i] * height());
        int y = height() - h;
        if(h == 0)
        {
            //QString infor =
                    //QString("h: %1 in DVNRecordWavingWidget::paintEvent").arg(h);
            //qDebug() << infor << "\n";
            h = 1;
            y = height() - 1;
        }
        painter.setPen(QPen(QColor(3 - static_cast<int>(i * dr)
                                   , 208 - static_cast<int>(i * dg)
                                   , 214 + static_cast<int>(i * db))));

        painter.drawRect(i * static_cast<int>(xStep), y, 1, h);
    }
}

void VNWaveform::resizeEvent(QResizeEvent *event)
{
    m_maxShowedSamples = event->size().width() / (WAVE_WIDTH*2);

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

    qDebug() << "scaleSamples:" << scaleSamples.size();
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

void VNWaveform::setSliderVisable(bool isVisable)
{
    //slider()->hide();
}
