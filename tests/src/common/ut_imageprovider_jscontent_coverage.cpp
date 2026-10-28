// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Coverage tests for previously-uncovered functions in imageprovider.cpp and
// jscontent.cpp. Focuses on executing function bodies without crashing.
// Build enables -fno-access-control, so the protected ImageProvider constructor
// and the private callJsSynchronous lambda can be exercised directly/indirectly.

#include "imageprovider.h"
#include "jscontent.h"
#include "web_engine_handler.h"

#include <gtest/gtest.h>

#include <QClipboard>
#include <QImage>
#include <QMimeData>
#include <QSize>
#include <QTest>
#include <QTimer>
#include <QVariant>

#include <functional>

#include "stub.h"

// ============================================================================
// ImageProvider coverage
// ============================================================================

TEST(ImageProviderCoverage, instance_returnsNonNullSingleton)
{
    ImageProvider *p1 = ImageProvider::instance();
    ASSERT_NE(nullptr, p1);
    // Singleton stability: same address across calls.
    EXPECT_EQ(p1, ImageProvider::instance());
}

TEST(ImageProviderCoverage, requestImage_returnsStoredImg)
{
    ImageProvider *provider = ImageProvider::instance();
    QSize reported;
    QImage img = provider->requestImage(QStringLiteral("any"), &reported, QSize(16, 16));
    // Default img is null; function body executes the return-by-member path.
    EXPECT_EQ(img.isNull(), provider->img.isNull());
}

TEST(ImageProviderCoverage, requestPixmap_executesFolderOperPath)
{
    ImageProvider *provider = ImageProvider::instance();
    QSize reported;
    // Index 1 is within the GlobalEnvent-populated icon range; out-of-range
    // indices are clamped to 0 inside getDefaultIcon.
    QPixmap pix = provider->requestPixmap(QStringLiteral("1"), &reported, QSize(16, 16));
    // No assertion on pixmap validity — the folder oper may yield a real or
    // null pixmap depending on icon presence; we only require no crash.
    SUCCEED();
}

TEST(ImageProviderCoverage, protectedConstructor_directConstruction)
{
    // -fno-access-control: protected constructor is reachable.
    ImageProvider *provider = new ImageProvider(QQuickImageProvider::Pixmap);
    ASSERT_NE(nullptr, provider);
    delete provider;
    // D0 (deleting destructor) and D2 (base object destructor) both fire on
    // `delete`; the destructor is `= default` and chains through QQuickImageProvider.
    SUCCEED();
}

TEST(ImageProviderCoverage, destructor_stackObject_D2path)
{
    // Stack-allocated destruction exercises the D2 base-object destructor path
    // (compiler-generated for `= default` virtual destructor) without going
    // through operator delete.
    {
        ImageProvider provider(QQuickImageProvider::Image);
        (void)provider;
    } // ~ImageProvider runs here.
    SUCCEED();
}

// ============================================================================
// JsContent coverage
// ============================================================================

TEST(JsContentCoverage, webPath_returnsFileUrl)
{
    const QString path = JsContent::instance()->webPath();
    EXPECT_TRUE(path.startsWith(QStringLiteral("file://"))) << path.toStdString();
    EXPECT_TRUE(path.contains(QStringLiteral("index.html"))) << path.toStdString();
}

TEST(JsContentCoverage, jsCallGetAppDataPath_returnsFileUrl)
{
    const QString url = JsContent::instance()->jsCallGetAppDataPath();
    // In the offscreen test env AppDataLocation is resolvable; mkpath is a no-op
    // when the dir already exists. Path must be a valid file:// URL.
    EXPECT_TRUE(url.startsWith(QStringLiteral("file://"))) << url.toStdString();
}

TEST(JsContentCoverage, jsCallSummernoteInitFinish_emitsLoadFinsh)
{
    // Verify signal emission by connecting a spy.
    JsContent *js = JsContent::instance();
    bool got = false;
    auto conn = QObject::connect(js, &JsContent::loadFinsh, [&]() { got = true; });
    js->jsCallSummernoteInitFinish();
    QObject::disconnect(conn);
    EXPECT_TRUE(got);
}

TEST(JsContentCoverage, jsCallDivTextTranslation_returnsJson)
{
    const QString json = JsContent::instance()->jsCallDivTextTranslation();
    EXPECT_FALSE(json.isEmpty());
    EXPECT_TRUE(json.contains(QStringLiteral("translateLabel"))) << json.toStdString();
}

TEST(JsContentCoverage, jsCallScrollChange_emitsAtTopAndNotTop)
{
    JsContent *js = JsContent::instance();

    bool isTopSignal = false;
    auto conn1 = QObject::connect(js, &JsContent::scrollTopChange, [&](const bool &v) {
        isTopSignal = v;
    });

    js->jsCallScrollChange(0);
    EXPECT_TRUE(isTopSignal);

    js->jsCallScrollChange(100);
    EXPECT_FALSE(isTopSignal);

    QObject::disconnect(conn1);
}

TEST(JsContentCoverage, jsCallPlayVoiceStop_emitsSignal)
{
    JsContent *js = JsContent::instance();
    bool got = false;
    auto conn = QObject::connect(js, &JsContent::playVoiceStop, [&]() { got = true; });
    js->jsCallPlayVoiceStop();
    QObject::disconnect(conn);
    EXPECT_TRUE(got);
}

TEST(JsContentCoverage, jsCallVoiceProgressChange_emitsWithMs)
{
    JsContent *js = JsContent::instance();
    qint64 reported = -1;
    auto conn = QObject::connect(js, &JsContent::playVoiceProgressChange, [&](qint64 v) {
        reported = v;
    });
    js->jsCallVoiceProgressChange(12345LL);
    QObject::disconnect(conn);
    EXPECT_EQ(12345LL, reported);
}

TEST(JsContentCoverage, jsCallSaveAudio_emitsSignal)
{
    JsContent *js = JsContent::instance();
    bool got = false;
    auto conn = QObject::connect(js, &JsContent::saveAudio, [&]() { got = true; });
    js->jsCallSaveAudio();
    QObject::disconnect(conn);
    EXPECT_TRUE(got);
}

TEST(JsContentCoverage, onClipChange_clipboardModeBranches)
{
    JsContent *js = JsContent::instance();

    // Ensure m_clipData differs from the current clipboard content so the
    // Clipboard branch emits callJsClipboardDataChanged. Previous tests may
    // have set m_clipData to the current clipboard via jsCallSetClipData.
    QClipboard *clip = QApplication::clipboard();
    const QMimeData *oldClipData = js->m_clipData;
    // QClipboard::setMimeData takes ownership (will delete the pointer), so
    // allocate on the heap — a stack QMimeData would be double-freed.
    QMimeData *tmpMime = new QMimeData;
    tmpMime->setText(QStringLiteral("ut-clip-reset"));
    clip->setMimeData(tmpMime);        // replaces clipboard; m_clipData is now stale
    // Note: the clipboard change triggers onClipChange via the
    // QClipboard::changed signal, which may update m_clipData.  Reset it
    // manually to force the next onClipChange to see a difference.
    js->m_clipData = nullptr;

    bool got = false;
    auto conn = QObject::connect(js, &JsContent::callJsClipboardDataChanged, [&]() { got = true; });

    js->onClipChange(QClipboard::Clipboard);  // primary branch — now mimeData() != m_clipData
    EXPECT_TRUE(got);

    got = false;
    js->onClipChange(QClipboard::Selection);  // non-Clipboard branch (no emit)
    EXPECT_FALSE(got);

    QObject::disconnect(conn);
    // Restore m_clipData so later tests are not affected.
    js->m_clipData = oldClipData;
}

// --- callJsSynchronous result round-trip ---
//
// QtWebEngine 退役后，jscontent.cpp 中的 QWebEnginePage 版 callJsSynchronous
// 已删除；同步调用链路收敛到 WebEngineHandler::callJsSynchronous()：
// 发出 requesetCallJsSynchronous → QML (WebEngineView.qml:724) 执行
// runJavaScript 并回调 onCallJsResult → 事件循环退出并返回结果。
// 这里用定时器模拟 QML 的异步回包。

TEST(JsContentCoverage, callJsSynchronousViaWebEngineHandler_roundTrip)
{
    WebEngineHandler h;
    QObject target;
    h.setTarget(&target);

    // 模拟 QML onRequesetCallJsSynchronous：异步回调 onCallJsResult。
    auto conn = QObject::connect(
        &h, &WebEngineHandler::requesetCallJsSynchronous, &h,
        [&h](const QString &) {
            QTimer::singleShot(0, [&h]() {
                h.onCallJsResult(QVariant(QStringLiteral("stub-result")));
            });
        },
        Qt::QueuedConnection);
    ASSERT_TRUE(conn);

    const QVariant result = h.callJsSynchronous(QStringLiteral("1+1"));
    EXPECT_EQ(QVariant(QStringLiteral("stub-result")), result);

    QObject::disconnect(conn);
}
