// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Coverage tests for previously-uncovered functions across migration
// view controller, qtplayer, tiptapchannelbridge, utils, and vlcplayer.
// Focuses on executing function bodies without crashing; stubs isolate
// risky DBus / file / thread operations.

#include "migrationviewcontroller.h"
#include "qtplayer.h"
#include "tiptapchannelbridge.h"
#include "utils.h"
#include "vlcplayer.h"

#include "importolddata/dbmigration/migrationorchestrator.h"
#include "importolddata/dbmigration/migrationstate.h"

#include <gtest/gtest.h>

#include <QBuffer>
#include <QImage>
#include <QObject>
#include <QVariant>

#include <vlc/vlc.h>
#include <vlc/libvlc_events.h>

#include "stub.h"

// ============================================================================
// MigrationViewController coverage
// ============================================================================

TEST(MigrationViewControllerCoverage, instance_returnsNonNullSingleton)
{
    MigrationViewController *p1 = MigrationViewController::instance();
    ASSERT_NE(nullptr, p1);
    EXPECT_EQ(p1, MigrationViewController::instance());
}

// start() normally would launch a real background migration thread via
// MigrationOrchestrator::startIfNeeded. To keep the unit test hermetic and
// side-effect-free, we stub startIfNeeded to return nullptr, which exercises
// the rollback path inside start() (setMigrationActive(false); return).
static MigrationOrchestrator *stub_startIfNeeded_returnsNull(QObject * /*consumer*/)
{
    return nullptr;
}

TEST(MigrationViewControllerCoverage, start_withNullOrchestrator_rollBackCleanly)
{
    Stub stub;
    stub.set(ADDR(MigrationOrchestrator, startIfNeeded),
             stub_startIfNeeded_returnsNull);

    // Bump migrationActive to force the early-return path first: covers the
    // "already in progress" branch.
    MigrationViewController *ctrl = MigrationViewController::instance();
    ctrl->m_migrationActive = true;          // -fno-access-control
    ctrl->start();                            // early-return: no state load
    EXPECT_TRUE(ctrl->m_migrationActive);

    // Now clear and let start() proceed through the state-load path. With the
    // stub, startIfNeeded returns nullptr and start() rolls back.
    ctrl->m_migrationActive = false;
    ctrl->m_orchestrator = nullptr;           // ensure early-return not taken
    ctrl->start();
    // After rollback migrationActive must be false.
    EXPECT_FALSE(ctrl->m_migrationActive);
}

// ============================================================================
// QtPlayer coverage
// ============================================================================

TEST(QtPlayerCoverage, destructor_deletingPath)
{
    // new + delete exercises the deleting destructor (D0) which deletes
    // m_player and m_audioOutput. Stack-allocated destruction also exercises
    // the base-object (D2) path through the virtual destructor chain.
    QtPlayer *player = new QtPlayer(nullptr);
    ASSERT_NE(nullptr, player);
    delete player;
    SUCCEED();
}

TEST(QtPlayerCoverage, destructor_stackObject_D2path)
{
    {
        QtPlayer player(nullptr);
        (void)player;
    }  // ~QtPlayer runs here.
    SUCCEED();
}

// ============================================================================
// TiptapChannelBridge coverage
// ============================================================================

TEST(TiptapChannelBridgeCoverage, jsPasteImage_invalidNoComma_emitsFailure)
{
    TiptapChannelBridge bridge;
    bool got = false;
    auto conn = QObject::connect(&bridge, &TiptapChannelBridge::insertImageFailed,
                                 [&](const QString &) { got = true; });
    // No comma in input -> early "invalid pasted image data" failure path.
    bridge.jsPasteImage(QStringLiteral("not-a-data-url"));
    QObject::disconnect(conn);
    EXPECT_TRUE(got);
}

TEST(TiptapChannelBridgeCoverage, jsPasteImage_unsupportedMime_emitsFailure)
{
    TiptapChannelBridge bridge;
    int failures = 0;
    auto conn = QObject::connect(&bridge, &TiptapChannelBridge::insertImageFailed,
                                 [&](const QString &) { ++failures; });
    // Comma present but mime type unsupported -> failure path.
    bridge.jsPasteImage(QStringLiteral("data:image/gif;base64,AAAA"));
    QObject::disconnect(conn);
    EXPECT_EQ(1, failures);
}

TEST(TiptapChannelBridgeCoverage, jsPasteImage_validPng_emitsInsertImage)
{
    TiptapChannelBridge bridge;
    QString insertedJson;
    auto conn = QObject::connect(&bridge, &TiptapChannelBridge::insertImage,
                                 [&](const QString &json) { insertedJson = json; });

    // Build a real 2x2 PNG, base64-encode it, and form a data URL.
    QImage img(2, 2, QImage::Format_RGB32);
    img.fill(Qt::red);
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    buf.close();
    const QString dataUrl = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(ba.toBase64());

    bridge.jsPasteImage(dataUrl);
    QObject::disconnect(conn);

    EXPECT_FALSE(insertedJson.isEmpty());
    EXPECT_TRUE(insertedJson.contains(QStringLiteral("relPath"))) << insertedJson.toStdString();
}

TEST(TiptapChannelBridgeCoverage, jsPasteImage_decodingFailure_emitsFailure)
{
    TiptapChannelBridge bridge;
    int failures = 0;
    auto conn = QObject::connect(&bridge, &TiptapChannelBridge::insertImageFailed,
                                 [&](const QString &) { ++failures; });
    // Valid mime but corrupt payload -> fromBase64 yields junk that QImage
    // cannot decode.
    bridge.jsPasteImage(QStringLiteral("data:image/png;base64,@@@@invalid@@@@"));
    QObject::disconnect(conn);
    EXPECT_EQ(1, failures);
}

// ============================================================================
// Utils coverage
// ============================================================================

TEST(UtilsCoverage, constructor_executesBody)
{
    // The Utils constructor only logs; constructing once executes the body.
    Utils u;
    (void)u;
    SUCCEED();
}

// ============================================================================
// VlcPlayer coverage
// ============================================================================

TEST(VlcPlayerCoverage, destructor_uninitializedInstance_noCrash)
{
    // Constructed without calling init(), m_vlcPlayer/m_vlcInst are null, so
    // deinit() is effectively a no-op. new + delete exercises the D0 deleting
    // destructor.
    VlcPlayer *player = new VlcPlayer(nullptr);
    ASSERT_NE(nullptr, player);
    delete player;
    SUCCEED();
}

TEST(VlcPlayerCoverage, destructor_stackObject_D2path)
{
    {
        VlcPlayer player(nullptr);
        (void)player;
    }  // ~VlcPlayer runs here.
    SUCCEED();
}

// --- VlcPlayer::handleEvent coverage ---
//
// handleEvent is a private static callback dispatched by libvlc. We invoke it
// directly (reachable thanks to -fno-access-control) with synthetic event
// structures, exercising each switch branch.

TEST(VlcPlayerCoverage, handleEvent_endReached_emitsPlayEnd)
{
    VlcPlayer player(nullptr);
    bool got = false;
    auto conn = QObject::connect(&player, &VlcPlayer::playEnd, [&]() { got = true; });

    // The libvlc_event_t struct begins with `int type; void *p_obj; union u;`.
    // For MediaPlayerEndReached no union member is accessed, so type alone is
    // sufficient. The local FakeEvent mirrors the layout we touch.
    struct FakeEvent {
        int type;
        void *p_obj;
        union {
            qint64 new_time;
            qint64 new_duration;
        } u;
    } event{};
    event.type = libvlc_MediaPlayerEndReached;
    event.u.new_time = 0;

    VlcPlayer::handleEvent(reinterpret_cast<const libvlc_event_t *>(&event), &player);
    QObject::disconnect(conn);
    EXPECT_TRUE(got);
}

TEST(VlcPlayerCoverage, handleEvent_timeChanged_emitsPositionChanged)
{
    VlcPlayer player(nullptr);
    qint64 reported = -1;
    auto conn = QObject::connect(&player, &VlcPlayer::positionChanged,
                                 [&](qint64 v) { reported = v; });

    struct FakeEvent {
        int type;
        void *p_obj;
        union {
            qint64 new_time;
            qint64 new_duration;
        } u;
    } event{};
    event.type = libvlc_MediaPlayerTimeChanged;
    event.u.new_time = 4242;

    VlcPlayer::handleEvent(reinterpret_cast<const libvlc_event_t *>(&event), &player);
    QObject::disconnect(conn);
    EXPECT_EQ(4242, reported);
}

TEST(VlcPlayerCoverage, handleEvent_lengthChanged_emitsDurationChanged)
{
    VlcPlayer player(nullptr);
    qint64 reported = -1;
    auto conn = QObject::connect(&player, &VlcPlayer::durationChanged,
                                 [&](qint64 v) { reported = v; });

    struct FakeEvent {
        int type;
        void *p_obj;
        union {
            qint64 new_time;
            qint64 new_duration;
        } u;
    } event{};
    event.type = libvlc_MediaPlayerLengthChanged;
    event.u.new_duration = 9090;

    VlcPlayer::handleEvent(reinterpret_cast<const libvlc_event_t *>(&event), &player);
    QObject::disconnect(conn);
    EXPECT_EQ(9090, reported);
}

TEST(VlcPlayerCoverage, handleEvent_defaultBranch_noOp)
{
    VlcPlayer player(nullptr);
    // Pick an unhandled type value to hit the default branch.
    struct FakeEvent {
        int type;
        void *p_obj;
        union {
            qint64 new_time;
            qint64 new_duration;
        } u;
    } event{};
    event.type = -1;  // not any known libvlc event type
    event.u.new_time = 0;

    VlcPlayer::handleEvent(reinterpret_cast<const libvlc_event_t *>(&event), &player);
    SUCCEED();  // default branch only logs
}
