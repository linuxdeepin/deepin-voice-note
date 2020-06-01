/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "vnoteaudiomanager.h"

#include <QAudioDeviceInfo>

#include <DLog>

#include <iostream>
using namespace std;

VNoteAudioManager* VNoteAudioManager::_instance = nullptr;

VNoteAudioManager::VNoteAudioManager(QObject *parent)
    : QObject(parent)
    , m_pAudioPlayer(new QMediaPlayer(this))
    , m_pAudioPlayerProbe(new QAudioProbe(this))
    , m_pAudioRecord(new QAudioRecorder(this))
    , m_pAudioRecProbe(new QAudioProbe(this))
{
    initAudio();
    initConnections();
}

VNoteAudioManager *VNoteAudioManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteAudioManager();
    }

    return  _instance;
}

void VNoteAudioManager::initAudio()
{
    QAudioDeviceInfo defaultDeviceInfo = QAudioDeviceInfo::defaultInputDevice();

    QAudioFormat useAudioFmt = defaultDeviceInfo.preferredFormat();

    if (useAudioFmt.sampleRate() > MaxSampleRate) {
        useAudioFmt.setSampleRate(MaxSampleRate);
    }

    if (useAudioFmt.channelCount() > MaxChannelCount) {
        useAudioFmt.setChannelCount(MaxChannelCount);
    }

    if (!defaultDeviceInfo.isFormatSupported(useAudioFmt)) {
        qWarning() << "Audio format:" << useAudioFmt
                   << " don't support by device:" << defaultDeviceInfo.deviceName();
        useAudioFmt = defaultDeviceInfo.nearestFormat(useAudioFmt);
    }

    qInfo() << "Actually used audio format:" << useAudioFmt;

    QStringList supportCodecs = m_pAudioRecord->supportedAudioCodecs();
    QStringList supportContainers = m_pAudioRecord->supportedContainers();

    //Get mp3 codec
    for (auto codec : supportCodecs) {
        if (codec.startsWith(m_defaultAudioFmt)) {
            m_audioCodec = codec;
            break;
        }
    }

    //Get mp3 codec container
    for (auto container : supportContainers) {
        if (container.startsWith(m_defaultAudioFmt)) {
            m_audioContainer = container;
        }
    }

    //Set Audio parameter
    m_audioEncoderSetting.setCodec(m_audioCodec);

    m_audioEncoderSetting.setQuality(QMultimedia::HighQuality);
    m_audioEncoderSetting.setEncodingMode(QMultimedia::ConstantQualityEncoding);
    m_audioEncoderSetting.setChannelCount(useAudioFmt.channelCount());
    m_audioEncoderSetting.setSampleRate(useAudioFmt.sampleRate());
    m_audioEncoderSetting.setBitRate(useAudioFmt.sampleSize());

    m_pAudioRecProbe->setSource(m_pAudioRecord.get());
    m_pAudioPlayerProbe->setSource(m_pAudioPlayer.get());

    qInfo() << "audio settings: {\n"
            << " codec=" << m_audioEncoderSetting.codec()
            << ", "      << m_audioEncoderSetting.sampleRate()
            << ", "      << m_audioEncoderSetting.bitRate()
            << ", channelCount=" << m_audioEncoderSetting.channelCount()
            << ", container =" << m_audioContainer
            << " }";
}

void VNoteAudioManager::updateAudioInputParam()
{
    //Set Audio Input
    if (m_pAudioRecord->state() == QMediaRecorder::StoppedState) {
        m_pAudioRecord->setAudioInput(m_pAudioRecord->defaultAudioInput());

        //Only need update settting when setting be changed.
        if (m_pAudioRecord->audioSettings() != m_audioEncoderSetting) {
            m_pAudioRecord->setEncodingSettings(m_audioEncoderSetting
                                                , QVideoEncoderSettings()
                                                , m_audioContainer);
            qInfo() << "Update recording settings: {\n"
                    << " codec=" << m_audioEncoderSetting.codec()
                    << ", "      << m_audioEncoderSetting.sampleRate()
                    << ", "      << m_audioEncoderSetting.bitRate()
                    << ", channelCount=" << m_audioEncoderSetting.channelCount()
                    << ", container =" << m_audioContainer
                    << " }";
        }
    } else {
        qInfo() << "Update audioInput encode setting failed. RecordState:"
                << m_pAudioRecord->state();
    }
}

void VNoteAudioManager::initConnections()
{
    connect(m_pAudioRecord.get(), &QAudioRecorder::durationChanged
            ,this, &VNoteAudioManager::recordDurationChanged);
    //Detect the recording error
    connect(m_pAudioRecord.get(), QOverload<QMediaRecorder::Error>::of(&QMediaRecorder::error),
            this, &VNoteAudioManager::onRecordError);
    connect(m_pAudioRecProbe.get(), &QAudioProbe::audioBufferProbed
            ,this, [this](const QAudioBuffer &buffer) {
        emit recAudioBufferProbed(buffer);
    });
}

void VNoteAudioManager::setPlayFileName(const QString &fileName)
{
    if (m_playFileName != fileName) {
        m_playFileName = fileName;
        m_pAudioPlayer->setMedia(QUrl::fromLocalFile(m_playFileName));
    }
}

void VNoteAudioManager::startPlay()
{
    int state = m_pAudioPlayer->state();
    if (QMediaPlayer::PlayingState != state) {
        //Restore the play position in pause state
//        if (QMediaPlayer::PausedState == state) {
//            m_pAudioPlayer->setPosition(m_playPosition);
//        }

        m_pAudioPlayer->play();
    }
}

void VNoteAudioManager::pausePlay()
{
    if (QMediaPlayer::PlayingState == m_pAudioPlayer->state()) {
        m_pAudioPlayer->pause();
        //m_playPosition = m_pAudioPlayer->position();
    }
}

void VNoteAudioManager::stopPlay()
{
    if (QMediaPlayer::StoppedState != m_pAudioPlayer->state()) {
        m_pAudioPlayer->stop();
    }
}

void VNoteAudioManager::setRecordFileName(const QString &fileName)
{
    if (QMediaRecorder::StoppedState == m_pAudioRecord->state()) {
        if (m_recordFileName != fileName) {
            m_recordFileName = fileName;
            m_pAudioRecord->setOutputLocation(QUrl::fromLocalFile(m_recordFileName));
        }
    } else {
        qCritical() << "Should not change record name during recording";
    }
}

void VNoteAudioManager::startRecord()
{
    if (QMediaRecorder::RecordingState != m_pAudioRecord->state()) {
        updateAudioInputParam();
        m_pAudioRecord->record();
    }
}

void VNoteAudioManager::pauseRecord()
{
    if (QMediaRecorder::RecordingState == m_pAudioRecord->state()) {
        m_pAudioRecord->pause();
    }
}

void VNoteAudioManager::stopRecord()
{
    if (QMediaRecorder::StoppedState != m_pAudioRecord->state()) {
        m_pAudioRecord->stop();
        m_recordFileName.clear();
    }
}

void VNoteAudioManager::recordDurationChanged(qint64 duration)
{
    emit recDurationChange(duration);
}

void VNoteAudioManager::onDefaultInputChanged(const QString &name)
{
    qInfo() << "Default input source changed to:" << name;

    initAudio();
}

void VNoteAudioManager::onRecordError(QMediaRecorder::Error error)
{
    qCritical() << "Recording error happed:******" << error;
}

QMediaPlayer* VNoteAudioManager::getPlayerObject()
{
    return m_pAudioPlayer.get();
}
