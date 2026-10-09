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

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================
#include <QElapsedTimer>
#include <QDBusInterface>
#include <QEventLoop>
#include <QTimer>

// BUG 300535: 首次启动时 UOS AI 状态探测应异步完成且只执行一次（1c05a9e4）。
// 修复后 checkUosAiExists 使用 std::once_flag + 线程池探测，结果经队列连接
// 写回 m_status；调用方不被阻塞，重复调用为 no-op。
// PMS: https://pms.uniontech.com/bug-view-300535.html  commit: 1c05a9e4
TEST(VTextSpeechAndTrUT, BUG300535_checkUosAiExistsAsyncNonBlocking)
{
    VTextSpeechAndTrManager *m = VTextSpeechAndTrManager::instance();
    ASSERT_NE(nullptr, m);

    QElapsedTimer timer;
    timer.start();
    m->checkUosAiExists();
    m->checkUosAiExists();   // 第二次调用应为 no-op（std::call_once）
    // 调用方不阻塞：两次调用远小于线程池内 DBus 超时的量级
    EXPECT_LT(timer.elapsed(), 1000);

    // 泵事件让队列连接的写回闭包执行
    QEventLoop loop;
    QTimer::singleShot(300, &loop, &QEventLoop::quit);
    loop.exec();

    const int st = static_cast<int>(m->m_status);
    EXPECT_GE(st, 0);
    EXPECT_LE(st, 6);
}

// copilotInstalled 对不可达服务应安全返回 NotInstalled（version reply 无效）
// PMS: https://pms.uniontech.com/bug-view-300535.html  commit: 1c05a9e4
TEST(VTextSpeechAndTrUT, BUG300535_copilotInstalledMissingService)
{
    auto fake = QSharedPointer<QDBusInterface>::create(
        QStringLiteral("com.ut.copilot.absent"),
        QStringLiteral("/ut/path"),
        QStringLiteral("ut.interface"));
    EXPECT_EQ(VTextSpeechAndTrManager::NotInstalled,
              VTextSpeechAndTrManager::copilotInstalled(fake));
}
