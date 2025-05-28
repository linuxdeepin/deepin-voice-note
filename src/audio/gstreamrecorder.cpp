// Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gstreamrecorder.h"
#include "common/utils.h"

#include <DLog>

static const QString mp3Encoder = "capsfilter caps=audio/x-raw,rate=44100,channels=2 ! lamemp3enc name=enc target=1 cbr=true bitrate=192";

/**
 * @brief bufferProbe
 * @param pad
 * @param info
 * @param user_data 用户数据
 * @return 处理结果
 */
GstPadProbeReturn bufferProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad);
    GstreamRecorder *recorder = static_cast<GstreamRecorder *>(user_data);
    if (GstBuffer *buffer = gst_pad_probe_info_get_buffer(info))
        return recorder->doBufferProbe(buffer) ? GST_PAD_PROBE_OK : GST_PAD_PROBE_DROP;
    return GST_PAD_PROBE_OK;
}

/**
 * @brief GstBusMessageCb
 * @param bus 总线
 * @param msg 信息
 * @param userdata 用户数据
 * @return true 成功
 */
gboolean GstBusMessageCb(GstBus *bus, GstMessage *msg, void *userdata)
{
    Q_UNUSED(bus);
    GstreamRecorder *recorder = reinterpret_cast<GstreamRecorder *>(userdata);
    return recorder->doBusMessage(msg);
}

/**
 * @brief GstreamRecorder::GstreamRecorder
 * @param parent
 */
GstreamRecorder::GstreamRecorder(QObject *parent)
    : QObject(parent)
{
    qDebug() << "Initializing GstreamRecorder...";
    // check if current in linglong, update GST_PLUGIN_PATH
    if (Utils::inLinglongEnv()) {
        qputenv("GST_PLUGIN_PATH", QByteArray(INSTALL_LIB_PATH) + QByteArray("/gstreamer-1.0"));
        qDebug() << "Running in Linglong environment, updated GST_PLUGIN_PATH";
    }

    gst_init(nullptr, nullptr);
    qDebug() << "GStreamer initialized successfully";
}

/**
 * @brief GstreamRecorder::createPipe
 * @return true成功
 */
bool GstreamRecorder::createPipe()
{
    qDebug() << "Creating GStreamer pipeline...";
    GstElement *audioSrc = nullptr; //声音采集设备
    GstElement *audioResample = nullptr; //重采样
    GstElement *audioConvert = nullptr; //格式转换
    GstElement *audioQueue = nullptr; //数据缓存
    GstElement *audioEncoder = nullptr; //编码器
    GstElement *audioOutput = nullptr; //输出文件
    //   回音消除与噪声抑制
    //   GstElement *audiowebrtcdsp = nullptr;
    //   GstElement *audiowebrtcechoprobe = nullptr;

    bool success = false;
    do {
        audioSrc = gst_element_factory_make("pulsesrc", "audioSrc");
        if (audioSrc == nullptr) {
            qCritical() << "Failed to create audio source element (pulsesrc)";
            break;
        }
        if (!m_currentDevice.isEmpty()) {
            g_object_set(reinterpret_cast<gpointer *>(audioSrc), "device", m_currentDevice.toLatin1().data(), nullptr);
            qDebug() << "Set audio source device to:" << m_currentDevice;
        }

        //       audiowebrtcdsp= gst_element_factory_make("webrtcdsp","audiowebrtcdsp");
        //       if(audiowebrtcdsp == nullptr){
        //           qCritical() << "audiowebrtcdsp make error";
        //           break;
        //       }
        //       audiowebrtcechoprobe = gst_element_factory_make("webrtcechoprobe","webrtcechoprobe");
        //       if(audiowebrtcechoprobe == nullptr){
        //           qCritical() << "audiowebrtcechoprobe make error";
        //           break;
        //       }
        //       g_object_set(audiowebrtcdsp, "probe", "webrtcechoprobe", nullptr);

        audioResample = gst_element_factory_make("audioresample", nullptr);
        if (audioResample == nullptr) {
            qCritical() << "Failed to create audio resample element";
            break;
        }
        audioConvert = gst_element_factory_make("audioconvert", "audioconvert");
        if (audioConvert == nullptr) {
            qCritical() << "Failed to create audio convert element";
            break;
        }
        audioQueue = gst_element_factory_make("queue", "audioqueue");
        if (audioQueue == nullptr) {
            qCritical() << "Failed to create audio queue element";
            break;
        }
        audioEncoder = gst_parse_bin_from_description(mp3Encoder.toLatin1().constData(),
                                                      true, nullptr);
        if (audioEncoder == nullptr) {
            qCritical() << "Failed to create audio encoder element with description:" << mp3Encoder;
            break;
        }
        GstPad *pad = gst_element_get_static_pad(audioEncoder, "sink");
        if (pad) {
            gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, bufferProbe, this, nullptr);
            gst_object_unref(pad);
            qDebug() << "Added buffer probe to encoder sink pad";
        } else {
            qCritical() << "Failed to get encoder sink pad";
            break;
        }
        audioOutput = gst_element_factory_make("filesink", "filesink");
        if (audioOutput == nullptr) {
            qCritical() << "Failed to create file sink element";
            break;
        }
        if (!m_outputFile.isEmpty()) {
            g_object_set(reinterpret_cast<gpointer *>(audioOutput), "location", m_outputFile.toLatin1().constData(), nullptr);
            qDebug() << "Set output file to:" << m_outputFile;
        }
        m_pipeline = gst_pipeline_new("deepin-voice-note");
        GstBus *bus = gst_pipeline_get_bus(reinterpret_cast<GstPipeline *>(m_pipeline));
        gst_bus_add_watch(bus, GstBusMessageCb, this);
        gst_object_unref(bus);
        qDebug() << "Created pipeline and added bus watch";

        gst_bin_add_many(reinterpret_cast<GstBin *>(m_pipeline), audioSrc,
                         /*audiowebrtcdsp,audiowebrtcechoprobe,*/
                         audioResample, audioConvert, audioQueue,
                         audioEncoder, audioOutput, nullptr);
        if (!gst_element_link_many(audioSrc,
                                   /*audiowebrtcdsp,audiowebrtcechoprobe,*/
                                   audioResample, audioConvert,
                                   audioQueue, audioEncoder,
                                   audioOutput, nullptr)) {
            objectUnref(m_pipeline);
            m_pipeline = nullptr;
            qCritical() << "Failed to link pipeline elements";
            return success;
        }
        qDebug() << "Successfully linked all pipeline elements";
        success = true;
    } while (!success);
    if (!success) {
        qWarning() << "Pipeline creation failed, cleaning up elements";
        objectUnref(audioSrc);
        objectUnref(audioResample);
        objectUnref(audioConvert);
        objectUnref(audioQueue);
        objectUnref(audioEncoder);
        objectUnref(audioOutput);
        //       objectUnref(audiowebrtcdsp);
        //       objectUnref(audiowebrtcechoprobe);
    }
    return success;
}

/**
 * @brief GstreamRecorder::deinit
 */
void GstreamRecorder::deinit()
{
    qDebug() << "Deinitializing GstreamRecorder...";
    stopRecord();
    objectUnref(m_pipeline);
    gst_deinit();
    qDebug() << "GstreamRecorder deinitialized";
}

/**
 * @brief GstreamRecorder::~GstreamRecorder
 */
GstreamRecorder::~GstreamRecorder()
{
    deinit();
}

/**
 * @brief GstreamRecorder::GetGstState
 * @param state 状态
 * @param pending
 */
void GstreamRecorder::GetGstState(int *state, int *pending)
{
    *state = GST_STATE_NULL;
    *pending = GST_STATE_NULL;
    if (m_pipeline == nullptr)
        return;
    gst_element_get_state(m_pipeline, reinterpret_cast<GstState *>(state),
                          reinterpret_cast<GstState *>(pending), 0);
}

/**
 * @brief GstreamRecorder::startRecord
 * @return  true成功
 */
bool GstreamRecorder::startRecord()
{
    qDebug() << "Starting recording...";
    if (m_pipeline == nullptr && !createPipe()) {
        qCritical() << "Failed to create pipeline for recording";
        return false;
    }

    if (!m_format.isValid()) {
        qDebug() << "Initializing audio format";
        initFormat();
    }

    int state = -1;
    int pending = -1;
    GetGstState(&state, &pending);
    if (state == GST_STATE_PLAYING) {
        qDebug() << "Pipeline already in playing state";
        return true;
    } else if (state == GST_STATE_PAUSED) {
        qDebug() << "Resuming from paused state";
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        return true;
    }
    if (gst_element_set_state(m_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qCritical() << "Failed to set pipeline to playing state";
        return false;
    }
    qDebug() << "Recording started successfully";
    return true;
}

/**
 * @brief GstreamRecorder::stopRecord
 */
void GstreamRecorder::stopRecord()
{
    qDebug() << "Stopping recording...";
    if (m_pipeline) {
        setStateToNull();
        qDebug() << "Recording stopped";
    }
}

/**
 * @brief GstreamRecorder::pauseRecord
 */
void GstreamRecorder::pauseRecord()
{
    qDebug() << "Pausing recording...";
    if (m_pipeline != nullptr) {
        int state = -1;
        int pending = -1;
        GetGstState(&state, &pending);

        if (state == GST_STATE_PLAYING) {
            gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
            qDebug() << "Recording paused";
        }
    }
}

/**
 * @brief GstreamRecorder::setDevice
 * @param device 设备名称
 */
void GstreamRecorder::setDevice(const QString &device)
{
    if (device != m_currentDevice) {
        qDebug() << "Changing audio device from" << m_currentDevice << "to" << device;
        m_currentDevice = device;
        if (m_pipeline != nullptr) {
            GstElement *audioSrc = gst_bin_get_by_name(reinterpret_cast<GstBin *>(m_pipeline), "audioSrc");
            g_object_set(reinterpret_cast<gpointer *>(audioSrc), "device", device.toLatin1().data(), nullptr);
        }
    }
}

/**
 * @brief GstreamRecorder::setOutputFile
 * @param path 录音文件路径
 */
void GstreamRecorder::setOutputFile(const QString &path)
{
    qDebug() << "Setting output file to:" << path;
    m_outputFile = path;
    if (m_pipeline != nullptr) {
        GstElement *audioSink = gst_bin_get_by_name(reinterpret_cast<GstBin *>(m_pipeline), "filesink");
        g_object_set(reinterpret_cast<gpointer *>(audioSink), "location", m_outputFile.toLatin1().constData(), nullptr);
    }
}

/**
 * @brief GstreamRecorder::doBusMessage
 * @param message 消息
 * @return true 成功
 */
bool GstreamRecorder::doBusMessage(GstMessage *message)
{
    if (!message)
        return true;
    switch (message->type) {
    case GST_MESSAGE_ERROR: {
        GError *error = nullptr;
        gchar *dbg = nullptr;

        gst_message_parse_error(message, &error, &dbg);

        if (dbg) {
            qDebug() << "Debug info:" << dbg;
            g_free(dbg);
        }

        if (error) {
            QString errMsg = error->message;
            emit errorMsg(errMsg);
            qCritical() << "Pipeline error:" << errMsg;
            g_error_free(error);
        }
        break;
    }
    default:
        break;
    }
    return true;
}

/**
 * @brief GstreamRecorder::doBufferProbe
 * @param buffer 录音数据
 * @return true 成功
 */
bool GstreamRecorder::doBufferProbe(GstBuffer *buffer)
{
    if (buffer) {
        qint64 position = static_cast<qint64>(buffer->pts);
        position = position >= 0
                       ? position / (1000 * 1000) // 毫秒
                       : -1;
        QByteArray data;
        GstMapInfo info;
        if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
            data = QByteArray(reinterpret_cast<const char *>(info.data),
                              static_cast<int>(info.size));
            gst_buffer_unmap(buffer, &info);
            qDebug() << "Processed audio buffer at position:" << position << "ms, size:" << data.size() << "bytes";
        } else {
            return true;
        }

        QMutexLocker locker(&m_bufferMutex);
        if (!m_pendingBuffer.isValid())
            QMetaObject::invokeMethod(this, "bufferProbed", Qt::QueuedConnection);
        m_pendingBuffer = QAudioBuffer(data, m_format, position);
    }
    return true;
}

/**
 * @brief GstreamRecorder::bufferProbed
 */
void GstreamRecorder::bufferProbed()
{
    QAudioBuffer audioBuffer;
    {
        QMutexLocker locker(&m_bufferMutex);
        if (!m_pendingBuffer.isValid())
            return;
        audioBuffer = m_pendingBuffer;
        m_pendingBuffer = QAudioBuffer();
    }
    emit audioBufferProbed(audioBuffer);
}

/**
 * @brief GstreamRecorder::setStateToNull
 */
void GstreamRecorder::setStateToNull()
{
    qDebug() << "Setting pipeline state to NULL...";
    GstState cur_state = GST_STATE_NULL, pending = GST_STATE_NULL;
    gst_element_get_state(m_pipeline, &cur_state, &pending, 0);
    if (cur_state == GST_STATE_NULL) {
        if (pending != GST_STATE_VOID_PENDING) {
            gst_element_set_state(m_pipeline, GST_STATE_NULL);
        }
        return;
    }
    gst_element_set_state(m_pipeline, GST_STATE_READY);
    gst_element_get_state(m_pipeline, nullptr, nullptr, static_cast<GstClockTime>(-1));
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    qDebug() << "Pipeline state set to NULL";
}

/**
 * @brief GstreamRecorder::initFormat
 */
void GstreamRecorder::initFormat()
{
    qDebug() << "Initializing audio format...";
    //通道，采样率
    m_format.setChannelCount(2);
    m_format.setSampleRate(44100);
    //lamemp3enc 编码器插件格式为S16LE
    m_format.setSampleFormat(QAudioFormat::Int16);
    qDebug() << "Audio format initialized:"
             << "\n  - Channels:" << m_format.channelCount()
             << "\n  - Sample rate:" << m_format.sampleRate()
             << "\n  - Sample format:" << m_format.sampleFormat();
}
/**
 * @brief GstreamRecorder::objectUnref
 * @param object 处理对象
 */
void GstreamRecorder::objectUnref(gpointer object)
{
    if (object != nullptr) {
        gst_object_unref(object);
        object = nullptr;
    }
}
