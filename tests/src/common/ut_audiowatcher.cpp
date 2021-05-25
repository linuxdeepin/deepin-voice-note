/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_audiowatcher.h"
#include "audiowatcher.h"
#include <QAction>

ut_audiowatcher_test::ut_audiowatcher_test()
{
}

void ut_audiowatcher_test::SetUp()
{
    m_AudioWatcher = new AudioWatcher;
}

void ut_audiowatcher_test::TearDown()
{
    delete m_AudioWatcher;
}

TEST_F(ut_audiowatcher_test, getDeviceName)
{
    m_AudioWatcher->getDeviceName(AudioWatcher::Internal);
    m_AudioWatcher->getDeviceName(AudioWatcher::Micphone);
}

TEST_F(ut_audiowatcher_test, getVolume)
{
    m_AudioWatcher->getVolume(AudioWatcher::Internal);
    m_AudioWatcher->getVolume(AudioWatcher::Micphone);
}

TEST_F(ut_audiowatcher_test, getMute)
{
    m_AudioWatcher->getMute(AudioWatcher::Internal);
    m_AudioWatcher->getMute(AudioWatcher::Micphone);
}

TEST_F(ut_audiowatcher_test, onDefaultSourceActivePortChanged)
{
    m_AudioWatcher->onDefaultSourceActivePortChanged(m_AudioWatcher->m_pDefaultSource->activePort());
}

TEST_F(ut_audiowatcher_test, onDefaultSinkActivePortChanged)
{
    m_AudioWatcher->onDefaultSinkActivePortChanged(m_AudioWatcher->m_pDefaultSink->activePort());
}

TEST_F(ut_audiowatcher_test, onDefaultSourceChanaged)
{
    AudioPort tmpAudioPort = m_AudioWatcher->m_pDefaultSource->activePort();
    QDBusObjectPath value("/com/deepin/daemon/Audio/Source2");
    m_AudioWatcher->onDefaultSourceChanaged(value);
}

TEST_F(ut_audiowatcher_test, onDefaultSinkChanaged)
{
    AudioPort tmpAudioPort = m_AudioWatcher->m_pDefaultSink->activePort();
    QDBusObjectPath value("/com/deepin/daemon/Audio/Sink2");
    m_AudioWatcher->onDefaultSinkChanaged(value);
}

TEST_F(ut_audiowatcher_test, onSourceVolumeChanged)
{
    double tmpdl = 0.8;
    m_AudioWatcher->onSourceVolumeChanged(tmpdl);

    AudioPort tmpAudioPort = m_AudioWatcher->m_pDefaultSource->activePort();
    tmpAudioPort.name = "test";
    m_AudioWatcher->m_inAudioPort = tmpAudioPort;
    m_AudioWatcher->onSourceVolumeChanged(tmpdl);
}

TEST_F(ut_audiowatcher_test, onSinkVolumeChanged)
{
    double tmpdl = 0.8;
    m_AudioWatcher->onSinkVolumeChanged(tmpdl);

    AudioPort tmpAudioPort = m_AudioWatcher->m_pDefaultSink->activePort();
    tmpAudioPort.name = "test";
    m_AudioWatcher->m_outAudioPort = tmpAudioPort;
    m_AudioWatcher->onSinkVolumeChanged(tmpdl);
}

TEST_F(ut_audiowatcher_test, onSourceMuteChanged)
{
    bool tmp = m_AudioWatcher->m_inAudioMute;

    if (!tmp) {
        m_AudioWatcher->onSourceMuteChanged(true);
    } else {
        m_AudioWatcher->onSourceMuteChanged(false);
    }
}

TEST_F(ut_audiowatcher_test, onSinkMuteChanged)
{
    bool tmp = m_AudioWatcher->m_outAudioMute;

    if (!tmp) {
        m_AudioWatcher->onSinkMuteChanged(true);
    } else {
        m_AudioWatcher->onSinkMuteChanged(false);
    }
}

TEST_F(ut_audiowatcher_test, enumtest)
{
    QAction *tmpact = new QAction("test");
    tmpact->setProperty("test", m_AudioWatcher->Internal);
    AudioWatcher::AudioMode tem = tmpact->property("test").value<AudioWatcher::AudioMode>();
    delete tmpact;
}
