// Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_vtextspeechandtrmanager.h"
#include "vtextspeechandtrmanager.h"

UT_VTextSpeechAndTrManager::UT_VTextSpeechAndTrManager()
{
}

void UT_VTextSpeechAndTrManager::SetUp()
{
    // 使用单例模式，无需手动创建实例
}

void UT_VTextSpeechAndTrManager::TearDown()
{
    // 使用单例模式，无需手动删除实例
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_getTextToSpeechEnable_001)
{
    VTextSpeechAndTrManager::instance()->getTextToSpeechEnable();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_getSpeechToTextEnable_001)
{
    VTextSpeechAndTrManager::instance()->getSpeechToTextEnable();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_onTextToSpeech_001)
{
    VTextSpeechAndTrManager::instance()->onTextToSpeech();
    VTextSpeechAndTrManager::instance()->onStopTextToSpeech();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_isTextToSpeechInWorking_001)
{
    VTextSpeechAndTrManager::instance()->onTextToSpeech();
    VTextSpeechAndTrManager::instance()->isTextToSpeechInWorking();
}

TEST_F(UT_VTextSpeechAndTrManager, UT_VTextSpeechAndTrManager_onSpeechToText_001)
{
    VTextSpeechAndTrManager::instance()->onSpeechToText();
}
