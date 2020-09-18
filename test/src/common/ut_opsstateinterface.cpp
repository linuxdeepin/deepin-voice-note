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

#include "ut_opsstateinterface.h"

#include "opsstateinterface.h"


ut_opsstateinterface_test::ut_opsstateinterface_test()
{

}

void ut_opsstateinterface_test::SetUp()
{
    m_opsstateinterface = new OpsStateInterface();
}

void ut_opsstateinterface_test::TearDown()
{
    delete m_opsstateinterface;
}

TEST_F(ut_opsstateinterface_test, isSearching)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateSearching, true);
    ASSERT_TRUE(m_opsstateinterface->isSearching());
}

TEST_F(ut_opsstateinterface_test, isRecording)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateRecording, true);
    ASSERT_TRUE(m_opsstateinterface->isRecording());
}

TEST_F(ut_opsstateinterface_test, isPlaying)
{
    m_opsstateinterface->operState(m_opsstateinterface->StatePlaying, true);
    ASSERT_TRUE(m_opsstateinterface->isPlaying());
}

TEST_F(ut_opsstateinterface_test, isVoice2Text)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateVoice2Text, true);
    ASSERT_TRUE(m_opsstateinterface->isVoice2Text());
}

TEST_F(ut_opsstateinterface_test, isAppQuit)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateAppQuit, true);
    ASSERT_TRUE(m_opsstateinterface->isAppQuit());
}

TEST_F(ut_opsstateinterface_test, isAiSrvExist)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateAISrvAvailable, true);
    ASSERT_TRUE(m_opsstateinterface->isAiSrvExist());
}









