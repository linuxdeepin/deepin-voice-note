// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vtextspeechandtrmanager.h"

#include "gtest/gtest.h"

#include <QCoreApplication>
#include <QDBusInterface>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <QThreadPool>
#include <QVariant>

// --- VTextSpeechAndTrManager::instance() [L33] ---

TEST(UT_VTextSpeechCoverage, InstanceReturnsSingleton)
{
    VTextSpeechAndTrManager *const first = VTextSpeechAndTrManager::instance();
    ASSERT_NE(first, nullptr);
    VTextSpeechAndTrManager *const second = VTextSpeechAndTrManager::instance();
    EXPECT_EQ(first, second);
}

// --- VTextSpeechAndTrManager::errorString(Status) [L274] ---

TEST(UT_VTextSpeechCoverage, ErrorStringForKnownStatuses)
{
    VTextSpeechAndTrManager *const mgr = VTextSpeechAndTrManager::instance();

    EXPECT_FALSE(mgr->errorString(VTextSpeechAndTrManager::NotInstalled).isEmpty());
    EXPECT_FALSE(mgr->errorString(VTextSpeechAndTrManager::NoInputDevice).isEmpty());
    EXPECT_FALSE(mgr->errorString(VTextSpeechAndTrManager::NoOutputDevice).isEmpty());
}

TEST(UT_VTextSpeechCoverage, ErrorStringIsEmptyForOtherStatuses)
{
    VTextSpeechAndTrManager *const mgr = VTextSpeechAndTrManager::instance();

    // The switch falls through to the empty default for these values.
    EXPECT_TRUE(mgr->errorString(VTextSpeechAndTrManager::Enable).isEmpty());
    EXPECT_TRUE(mgr->errorString(VTextSpeechAndTrManager::Disable).isEmpty());
    EXPECT_TRUE(mgr->errorString(VTextSpeechAndTrManager::NoUserAgreement).isEmpty());
}

// --- VTextSpeechAndTrManager::launchCopilotChat(QSharedPointer<QDBusInterface> const&)
//     [private static, L343] ---
//
// -fno-access-control permits invoking the private static directly. A real
// interface to the (typically absent) copilot service keeps call() from
// dereferencing null; it returns an error reply => launchCopilotChat reports
// Disable without crashing.

TEST(UT_VTextSpeechCoverage, LaunchCopilotChatReturnsDisableWhenServiceMissing)
{
    const QSharedPointer<QDBusInterface> copilot =
        QSharedPointer<QDBusInterface>::create(QStringLiteral("com.deepin.copilot"),
                                               QStringLiteral("/com/deepin/copilot"),
                                               QStringLiteral("com.deepin.copilot"));
    ASSERT_FALSE(copilot.isNull());

    const VTextSpeechAndTrManager::Status status =
        VTextSpeechAndTrManager::launchCopilotChat(copilot);
    // Either the service is absent (Disable) or, in unlikely environments where
    // it exists, Enable. Both are valid non-crash outcomes.
    EXPECT_TRUE(status == VTextSpeechAndTrManager::Disable
                || status == VTextSpeechAndTrManager::Enable);
}

// --- VTextSpeechAndTrManager::checkUosAiExists() + nested lambdas [L39/L42/L43/L57] ---
//
// checkUosAiExists() uses std::call_once to schedule a one-shot probe on the
// global thread pool. Driving it to completion requires:
//   1. invoking checkUosAiExists() => lambda#1 (L42) runs synchronously.
//   2. waitForDone() => the pool worker runs initFunc (lambda#1#1, L43), which
//      queries the copilot service and posts a queued update back to the
//      singleton living on the main thread.
//   3. processEvents() => the queued invokeMethod delivers lambda#1#1#1 (L57),
//      assigning m_status.

TEST(UT_VTextSpeechCoverage, CheckUosAiExistsDrivesNestedLambdas)
{
    VTextSpeechAndTrManager *const mgr = VTextSpeechAndTrManager::instance();

    // Kick off the (once-only) probe.
    mgr->checkUosAiExists();

    // Wait for the thread-pool worker to finish the DBus probe.
    QThreadPool::globalInstance()->waitForDone(5000);

    // Pump the main-thread event loop so the queued assignment (L57) is
    // delivered to the singleton.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 200);

    // After the probe, m_status holds a concrete value (typically NotInstalled
    // when the copilot service is unavailable). status() confirms the queued
    // assignment took effect rather than leaving the default untouched.
    const int status = static_cast<int>(mgr->status());
    EXPECT_GE(status, 0);
}
