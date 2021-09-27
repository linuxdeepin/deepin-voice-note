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

UT_OpsStateInterface::UT_OpsStateInterface()
{
}

void UT_OpsStateInterface::SetUp()
{
    m_opsstateinterface = new OpsStateInterface();
}

void UT_OpsStateInterface::TearDown()
{
    delete m_opsstateinterface;
}

TEST_F(UT_OpsStateInterface, UT_OpsStateInterface_isSearching_001)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateSearching, true);
    EXPECT_TRUE(m_opsstateinterface->isSearching());
}

TEST_F(UT_OpsStateInterface, UT_OpsStateInterface_isRecording_001)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateRecording, true);
    EXPECT_TRUE(m_opsstateinterface->isRecording());
}

TEST_F(UT_OpsStateInterface, UT_OpsStateInterface_isPlaying_001)
{
    m_opsstateinterface->operState(m_opsstateinterface->StatePlaying, true);
    EXPECT_TRUE(m_opsstateinterface->isPlaying());
}

TEST_F(UT_OpsStateInterface, UT_OpsStateInterface_isVoice2Text_001)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateVoice2Text, true);
    EXPECT_TRUE(m_opsstateinterface->isVoice2Text());
}

TEST_F(UT_OpsStateInterface, UT_OpsStateInterface_isAppQuit_001)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateAppQuit, true);
    EXPECT_TRUE(m_opsstateinterface->isAppQuit());
}

TEST_F(UT_OpsStateInterface, UT_OpsStateInterface_isAiSrvExist_001)
{
    m_opsstateinterface->operState(m_opsstateinterface->StateAISrvAvailable, true);
    EXPECT_TRUE(m_opsstateinterface->isAiSrvExist());
}
