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

#include <gtest/gtest.h>

#include <QImage>
#include <QSize>
#include <QVariant>
#include <QWebEnginePage>

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
    // The Clipboard branch compares QApplication::clipboard()->mimeData() with
    // the cached m_clipData; when they differ (the default after init) the
    // callJsClipboardDataChanged signal is emitted.
    bool got = false;
    auto conn = QObject::connect(js, &JsContent::callJsClipboardDataChanged, [&]() { got = true; });

    js->onClipChange(QClipboard::Clipboard);  // primary branch
    EXPECT_TRUE(got);

    got = false;
    js->onClipChange(QClipboard::Selection);  // non-Clipboard branch (no emit)
    EXPECT_FALSE(got);

    QObject::disconnect(conn);
}

// --- callJsSynchronous inner lambda coverage ---
//
// The lambda at jscontent.cpp:271 only runs when runJavaScript actually
// invokes its callback. We stub the 2-arg runJavaScript overload so the stub
// synchronously dispatches the callback, which executes the lambda body
// (synResult = result; synLoop.quit()) inside callJsSynchronous. In this
// Qt6 build the callback type is std::function<void(const QVariant&)>.

static int g_stubCallbackInvocations = 0;
// First positional arg corresponds to the `this` pointer at the ABI level
// (member function call), so it is declared as void* and ignored. The other
// two args match runJavaScript's (script, callback) signature.
static void stub_runJavaScript_invokeCallback(void * /*thisPtr*/,
                                              const QString & /*script*/,
                                              const std::function<void(const QVariant &)> &callback)
{
    ++g_stubCallbackInvocations;
    // Invoking the callback runs the lambda captured inside callJsSynchronous.
    callback(QVariant(QStringLiteral("stub-result")));
}

TEST(JsContentCoverage, callJsSynchronous_innerLambdaExecuted)
{
    Stub stub;
    g_stubCallbackInvocations = 0;
    stub.set((void (QWebEnginePage::*)(const QString &,
                                       const std::function<void(const QVariant &)> &))
                 ADDR(QWebEnginePage, runJavaScript),
             stub_runJavaScript_invokeCallback);

    QWebEnginePage page;
    const QVariant result = JsContent::instance()->callJsSynchronous(
        &page, QStringLiteral("1+1"));
    // The stub synchronously invoked the lambda -> synResult was set, then
    // synLoop.quit() fired (queued until synLoop.exec() runs).
    EXPECT_GT(g_stubCallbackInvocations, 0);
    // Result returned from callJsSynchronous equals what the stub passed.
    EXPECT_EQ(QVariant(QStringLiteral("stub-result")), result);
}
