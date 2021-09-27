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

#include "ut_vtextspeechandtrmanager.h"
#include "vtextspeechandtrmanager.h"

UT_VTextSpeechAndTrManager::UT_VTextSpeechAndTrManager()
{
}

void UT_VTextSpeechAndTrManager::SetUp()
{
    m_vtextspeechandtrmanager = new VTextSpeechAndTrManager;
}

void UT_VTextSpeechAndTrManager::TearDown()
{
    delete m_vtextspeechandtrmanager;
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_getTextToSpeechEnable_001)
{
    m_vtextspeechandtrmanager->getTextToSpeechEnable();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_getSpeechToTextEnable_001)
{
    m_vtextspeechandtrmanager->getSpeechToTextEnable();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_getTransEnable_001)
{
    m_vtextspeechandtrmanager->getTransEnable();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_onTextToSpeech_001)
{
    m_vtextspeechandtrmanager->onTextToSpeech();
    m_vtextspeechandtrmanager->onStopTextToSpeech();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_isTextToSpeechInWorking_001)
{
    m_vtextspeechandtrmanager->onTextToSpeech();
    m_vtextspeechandtrmanager->isTextToSpeechInWorking();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_onSpeechToText_001)
{
    m_vtextspeechandtrmanager->onSpeechToText();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_onTextTranslate_001)
{
    m_vtextspeechandtrmanager->onTextTranslate();
}
