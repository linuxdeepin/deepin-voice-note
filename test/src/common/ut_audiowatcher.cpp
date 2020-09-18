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

#define private public
#define protected public
#include "audiowatcher.h"
#undef private
#undef protected

#include <QAction>

ut_audiowatcher_test::ut_audiowatcher_test()
{

}

TEST_F(ut_audiowatcher_test, getDeviceName)
{
    AudioWatcher audiowatcher;
    ASSERT_TRUE(audiowatcher.getDeviceName(audiowatcher.Internal).contains(".monitor"));
    ASSERT_FALSE(audiowatcher.getDeviceName(audiowatcher.Micphone).isEmpty());
}

TEST_F(ut_audiowatcher_test, getVolume)
{
    AudioWatcher audiowatcher;
    ASSERT_GE(audiowatcher.getVolume(audiowatcher.Internal), 0.0001);
    ASSERT_GE(audiowatcher.getVolume(audiowatcher.Micphone), 0.0001);
}

TEST_F(ut_audiowatcher_test, getMute)
{
    AudioWatcher audiowatcher;
    if (audiowatcher.m_inAudioMute) {
        ASSERT_TRUE(audiowatcher.getMute(audiowatcher.Micphone));
    }
    else {
        ASSERT_FALSE(audiowatcher.getMute(audiowatcher.Micphone));
    }

    if (audiowatcher.m_outAudioMute) {
        ASSERT_TRUE(audiowatcher.getMute(audiowatcher.Internal));
    }
    else {
        ASSERT_FALSE(audiowatcher.getMute(audiowatcher.Internal));
    }
}

TEST_F(ut_audiowatcher_test, onDefaultSourceActivePortChanged)
{
    AudioWatcher audiowatcher;
    audiowatcher.onDefaultSourceActivePortChanged(audiowatcher.m_pDefaultSource->activePort());
    ASSERT_EQ(audiowatcher.m_pDefaultSource->activePort(), audiowatcher.m_inAudioPort);
}

TEST_F(ut_audiowatcher_test, onDefaultSinkActivePortChanged)
{
    AudioWatcher audiowatcher;
    audiowatcher.onDefaultSinkActivePortChanged(audiowatcher.m_pDefaultSink->activePort());
    ASSERT_EQ(audiowatcher.m_pDefaultSink->activePort(), audiowatcher.m_outAudioPort);
}

TEST_F(ut_audiowatcher_test, onDefaultSourceChanaged)
{
    AudioWatcher audiowatcher;
    AudioPort tmpAudioPort = audiowatcher.m_pDefaultSource->activePort();
    QDBusObjectPath value("/com/deepin/daemon/Audio/Source2");
    audiowatcher.onDefaultSourceChanaged(value);
//    ASSERT_NE(tmpAudioPort, audiowatcher.m_inAudioPort);
}

TEST_F(ut_audiowatcher_test, onDefaultSinkChanaged)
{
    AudioWatcher audiowatcher;
    AudioPort tmpAudioPort = audiowatcher.m_pDefaultSink->activePort();
    QDBusObjectPath value("/com/deepin/daemon/Audio/Sink2");
    audiowatcher.onDefaultSinkChanaged(value);
    ASSERT_NE(tmpAudioPort, audiowatcher.m_outAudioPort);
}

TEST_F(ut_audiowatcher_test, onSourceVolumeChanged)
{
    AudioWatcher audiowatcher;
    double tmpdl = 0.8;
    double testinAudioPortVolume = audiowatcher.m_inAudioPortVolume;
    audiowatcher.onSourceVolumeChanged(tmpdl);
    ASSERT_NE(testinAudioPortVolume, audiowatcher.m_inAudioPortVolume);

    AudioPort tmpAudioPort = audiowatcher.m_pDefaultSource->activePort();
    tmpAudioPort.name = "test";
    audiowatcher.m_inAudioPort = tmpAudioPort;
    audiowatcher.onSourceVolumeChanged(tmpdl);
    ASSERT_NE(audiowatcher.m_inAudioPort, tmpAudioPort);
}

TEST_F(ut_audiowatcher_test, onSinkVolumeChanged)
{
    AudioWatcher audiowatcher;
    double tmpdl = 0.8;
    double testinAudioPortVolume = audiowatcher.m_outAudioPortVolume;
    audiowatcher.onSinkVolumeChanged(tmpdl);
    ASSERT_NE(testinAudioPortVolume, audiowatcher.m_outAudioPortVolume);

    AudioPort tmpAudioPort = audiowatcher.m_pDefaultSink->activePort();
    tmpAudioPort.name = "test";
    audiowatcher.m_outAudioPort = tmpAudioPort;
    audiowatcher.onSinkVolumeChanged(tmpdl);
    ASSERT_NE(audiowatcher.m_outAudioPort, tmpAudioPort);
}

TEST_F(ut_audiowatcher_test, onSourceMuteChanged)
{
    AudioWatcher audiowatcher;
    bool tmp = audiowatcher.m_inAudioMute;

    if (!tmp) {
        audiowatcher.onSourceMuteChanged(true);
        ASSERT_TRUE(audiowatcher.m_inAudioMute);
    }
    else {
        audiowatcher.onSourceMuteChanged(false);
        ASSERT_FALSE(audiowatcher.m_inAudioMute);
    }
}

TEST_F(ut_audiowatcher_test, onSinkMuteChanged)
{
    AudioWatcher audiowatcher;
    bool tmp = audiowatcher.m_outAudioMute;

    if (!tmp) {
        audiowatcher.onSinkMuteChanged(true);
        ASSERT_TRUE(audiowatcher.m_outAudioMute);
    }
    else {
        audiowatcher.onSinkMuteChanged(false);
        ASSERT_FALSE(audiowatcher.m_outAudioMute);
    }
}

TEST_F(ut_audiowatcher_test, enumtest)
{
    AudioWatcher audiowatcher;
    QAction* tmpact = new QAction("test");
    tmpact->setProperty("test", audiowatcher.Internal);
    AudioWatcher::AudioMode tem = tmpact->property("test").value<AudioWatcher::AudioMode>();
    ASSERT_EQ(audiowatcher.Internal, tem);
}
