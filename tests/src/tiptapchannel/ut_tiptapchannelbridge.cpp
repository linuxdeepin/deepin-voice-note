// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_tiptapchannelbridge.h"
#include "tiptapchannelbridge.h"

#include <QResource>
#include <QFile>
#include <QSignalSpy>
#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

UT_TiptapChannelBridge::UT_TiptapChannelBridge()
{
}

// ---------------------------------------------------------------------------
// 命名稳定契约：五类事件 signal 存在性与命名
// ---------------------------------------------------------------------------

// editorReady signal 存在
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_signal_editorReady_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::editorReady);
    bridge.jsEditorReady();
    EXPECT_EQ(spy.count(), 1);
}

// contentChanged signal 存在
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_signal_contentChanged_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::contentChanged);
    bridge.jsContentChanged();
    EXPECT_EQ(spy.count(), 1);
}

// requestContent signal 存在
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_signal_requestContent_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::requestContent);
    bridge.requestEditorContent();
    EXPECT_EQ(spy.count(), 1);
}

// contentSaved signal 存在且携带 envelopeJson
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_signal_contentSaved_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::contentSaved);
    QString envelope = "{\"format\":\"tiptap\",\"schemaVersion\":1,\"content\":{\"type\":\"doc\",\"content\":[{\"type\":\"paragraph\"}]}}";
    bridge.jsContentSaved(envelope);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), envelope);
}

// insertImage + insertImageFailed signal 存在
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_signal_insertImage_001)
{
    TiptapChannelBridge bridge;
    bridge.jsEditorReady();
    QSignalSpy insertSpy(&bridge, &TiptapChannelBridge::insertImage);
    bridge.sendInsertImage("{\"relPath\":\"images/test.png\"}");
    EXPECT_EQ(insertSpy.count(), 1);

    QSignalSpy failSpy(&bridge, &TiptapChannelBridge::insertImageFailed);
    bridge.jsInsertImageFailed("test reason");
    EXPECT_EQ(failSpy.count(), 1);
    EXPECT_EQ(failSpy.takeFirst().at(0).toString(), "test reason");
}

// insertVoiceBlock + insertVoiceBlockFailed signal 存在
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_signal_insertVoiceBlock_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy insertSpy(&bridge, &TiptapChannelBridge::insertVoiceBlock);
    bridge.sendInsertVoiceBlock("{\"voiceId\":\"v1\",\"voicePath\":\"voice/v1.wav\"}");
    EXPECT_EQ(insertSpy.count(), 1);

    QSignalSpy failSpy(&bridge, &TiptapChannelBridge::insertVoiceBlockFailed);
    bridge.jsInsertVoiceBlockFailed("voice fail");
    EXPECT_EQ(failSpy.count(), 1);
    EXPECT_EQ(failSpy.takeFirst().at(0).toString(), "voice fail");
}

// ---------------------------------------------------------------------------
// 图片 UI 交互：工具栏图片选择请求
// ---------------------------------------------------------------------------

TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestPickImage_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::pickImageRequested);
    bridge.jsRequestPickImage();
    EXPECT_EQ(spy.count(), 1);
}


TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsCopyPlainTextToClipboard_001)
{
    TiptapChannelBridge bridge;
    bridge.jsCopyPlainTextToClipboard(QStringLiteral("转写复制文本"));
    ASSERT_NE(QApplication::clipboard(), nullptr);
    EXPECT_EQ(QApplication::clipboard()->text(), QStringLiteral("转写复制文本"));
}

// 双击查看原图：相对路径归一化后下发本地路径
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestViewPicture_safe_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::viewPictureRequested);
    bridge.jsRequestViewPicture(QStringLiteral("images/photo.png"));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toString().endsWith(QStringLiteral("images/photo.png")));
}

// 双击查看原图：远程 / 越界路径拒绝下发
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestViewPicture_unsafe_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::viewPictureRequested);
    bridge.jsRequestViewPicture(QStringLiteral("https://example.com/x.png"));
    EXPECT_EQ(spy.count(), 0);
    bridge.jsRequestViewPicture(QStringLiteral("/etc/passwd"));
    EXPECT_EQ(spy.count(), 0);
}

// ---------------------------------------------------------------------------
// 保存 envelope 合法性
// ---------------------------------------------------------------------------

// contentSaved 携带合法 envelope（format=tiptap, schemaVersion=1, content.type=doc）
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_saveEnvelope_valid_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::contentSaved);

    QString envelopeJson = "{\"format\":\"tiptap\",\"schemaVersion\":1,\"content\":{\"type\":\"doc\",\"content\":[{\"type\":\"paragraph\"}]}}";
    bridge.jsContentSaved(envelopeJson);

    auto args = spy.takeFirst();
    QString received = args.at(0).toString();
    auto doc = QJsonDocument::fromJson(received.toUtf8());
    ASSERT_TRUE(doc.isObject());
    auto obj = doc.object();
    EXPECT_EQ(obj.value("format").toString(), "tiptap");
    EXPECT_EQ(obj.value("schemaVersion").toInt(), 1);
    auto content = obj.value("content").toObject();
    EXPECT_EQ(content.value("type").toString(), "doc");
}

// ---------------------------------------------------------------------------
// 空文档回告：单空段落标准 envelope
// ---------------------------------------------------------------------------

TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emptyDoc_envelope_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::contentSaved);

    // 空文档标准 envelope：单空段落
    QString emptyEnvelope = "{\"format\":\"tiptap\",\"schemaVersion\":1,\"content\":{\"type\":\"doc\",\"content\":[{\"type\":\"paragraph\"}]}}";
    bridge.jsContentSaved(emptyEnvelope);

    auto args = spy.takeFirst();
    auto doc = QJsonDocument::fromJson(args.at(0).toString().toUtf8());
    auto content = doc.object().value("content").toObject();
    auto paragraphs = content.value("content").toArray();
    EXPECT_EQ(paragraphs.size(), 1);
    EXPECT_EQ(paragraphs.at(0).toObject().value("type").toString(), "paragraph");
}

// ---------------------------------------------------------------------------
// 加载就绪缓存语义（R6）：未 editorReady 缓存，就绪后补发，不丢数据
// ---------------------------------------------------------------------------

// 未就绪时 loadEnvelope 缓存，不立即下发
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_cache_beforeReady_001)
{
    TiptapChannelBridge bridge;
    EXPECT_FALSE(bridge.isEditorReady());

    QSignalSpy loadSpy(&bridge, &TiptapChannelBridge::loadEnvelopeRequested);
    QString envelope = "{\"format\":\"tiptap\",\"schemaVersion\":1,\"content\":{\"type\":\"doc\",\"content\":[{\"type\":\"paragraph\"}]}}";
    bridge.loadEnvelope(envelope);

    // 未就绪：不应立即 emit loadEnvelopeRequested
    EXPECT_EQ(loadSpy.count(), 0);
    // 但应缓存
    EXPECT_EQ(bridge.pendingEnvelope(), envelope);
}

// 就绪后补发缓存的 envelope，不丢数据
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_replay_afterReady_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy loadSpy(&bridge, &TiptapChannelBridge::loadEnvelopeRequested);

    QString envelope = "{\"format\":\"tiptap\",\"schemaVersion\":1,\"content\":{\"type\":\"doc\",\"content\":[{\"type\":\"paragraph\",\"content\":[{\"type\":\"text\",\"text\":\"hello\"}]}]}}";

    // 未就绪时缓存
    bridge.loadEnvelope(envelope);
    EXPECT_EQ(loadSpy.count(), 0);

    // 就绪后补发
    bridge.jsEditorReady();
    EXPECT_EQ(loadSpy.count(), 1);
    EXPECT_EQ(loadSpy.takeFirst().at(0).toString(), envelope);
    EXPECT_TRUE(bridge.pendingEnvelope().isEmpty());
    EXPECT_TRUE(bridge.isEditorReady());
}

// 图片在 editorReady 前下发时缓存，避免 QWebChannel 尚未 connect 时丢失
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_insertImage_cache_beforeReady_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy insertSpy(&bridge, &TiptapChannelBridge::insertImage);
    const QString image = "{\"relPath\":\"images/test.png\"}";

    bridge.sendInsertImage(image);
    EXPECT_EQ(insertSpy.count(), 0);

    bridge.jsEditorReady();
    ASSERT_EQ(insertSpy.count(), 1);
    EXPECT_EQ(insertSpy.takeFirst().at(0).toString(), image);
}

// 就绪后直接下发，不缓存
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_direct_afterReady_001)
{
    TiptapChannelBridge bridge;
    bridge.jsEditorReady();  // 先就绪

    QSignalSpy loadSpy(&bridge, &TiptapChannelBridge::loadEnvelopeRequested);
    QString envelope = "{\"format\":\"tiptap\",\"schemaVersion\":1,\"content\":{\"type\":\"doc\"}}";
    bridge.loadEnvelope(envelope);

    EXPECT_EQ(loadSpy.count(), 1);
    EXPECT_EQ(loadSpy.takeFirst().at(0).toString(), envelope);
    EXPECT_TRUE(bridge.pendingEnvelope().isEmpty());
}

// ---------------------------------------------------------------------------
// 适配层方法
// ---------------------------------------------------------------------------

// resourceBaseUrl 返回 file:// 开头的 WEB_PATH
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_resourceBaseUrl_001)
{
    TiptapChannelBridge bridge;
    QString url = bridge.resourceBaseUrl();
    EXPECT_TRUE(url.startsWith("file://")) << url.toStdString();
}

// tiptapHtmlPath 优先返回 file://，文件不存在时回退 qrc
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_tiptapHtmlPath_001)
{
    TiptapChannelBridge bridge;
    QString path = bridge.tiptapHtmlPath();
    if (QFile::exists(QStringLiteral(TIPTAP_WEB_PATH "/tiptap-editor.html"))) {
        EXPECT_TRUE(path.startsWith("file://")) << path.toStdString();
    } else {
        EXPECT_TRUE(path.startsWith("qrc:")) << path.toStdString();
    }
    EXPECT_TRUE(path.contains("tiptap-editor.html")) << path.toStdString();
}

// Tiptap 默认启用，显式环境变量可临时回退 Summernote。
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_tiptapEnabled_001)
{
    const bool hadDisable = qEnvironmentVariableIsSet("DVN_TIPTAP_DISABLE");
    const bool hadLegacy = qEnvironmentVariableIsSet("DVN_SUMMERNOTE_LEGACY");
    const bool hadDebug = qEnvironmentVariableIsSet("DVN_TIPTAP_DEBUG");
    const QByteArray oldDisable = qgetenv("DVN_TIPTAP_DISABLE");
    const QByteArray oldLegacy = qgetenv("DVN_SUMMERNOTE_LEGACY");
    const QByteArray oldDebug = qgetenv("DVN_TIPTAP_DEBUG");

    auto restoreEnv = [&]() {
        hadDisable ? qputenv("DVN_TIPTAP_DISABLE", oldDisable) : qunsetenv("DVN_TIPTAP_DISABLE");
        hadLegacy ? qputenv("DVN_SUMMERNOTE_LEGACY", oldLegacy) : qunsetenv("DVN_SUMMERNOTE_LEGACY");
        hadDebug ? qputenv("DVN_TIPTAP_DEBUG", oldDebug) : qunsetenv("DVN_TIPTAP_DEBUG");
    };

    qunsetenv("DVN_TIPTAP_DISABLE");
    qunsetenv("DVN_SUMMERNOTE_LEGACY");
    qunsetenv("DVN_TIPTAP_DEBUG");

    TiptapChannelBridge defaultBridge;
    EXPECT_TRUE(defaultBridge.tiptapEnabled());
    EXPECT_TRUE(defaultBridge.debugEnabled());

    qputenv("DVN_TIPTAP_DISABLE", "1");
    TiptapChannelBridge disabledBridge;
    EXPECT_FALSE(disabledBridge.tiptapEnabled());
    EXPECT_FALSE(disabledBridge.debugEnabled());

    qputenv("DVN_TIPTAP_DISABLE", "0");
    qputenv("DVN_SUMMERNOTE_LEGACY", "true");
    TiptapChannelBridge legacyBridge;
    EXPECT_FALSE(legacyBridge.tiptapEnabled());

    qputenv("DVN_SUMMERNOTE_LEGACY", "0");
    qputenv("DVN_TIPTAP_DEBUG", "0");
    TiptapChannelBridge legacyDebugBridge;
    EXPECT_FALSE(legacyDebugBridge.tiptapEnabled());

    qputenv("DVN_TIPTAP_DEBUG", "1");
    TiptapChannelBridge explicitBridge;
    EXPECT_TRUE(explicitBridge.tiptapEnabled());

    restoreEnv();
}

// ---------------------------------------------------------------------------
// 字体列表下发（fontListProvided）：缓存 / 补发 / 直接下发
// ---------------------------------------------------------------------------

// 未就绪时 sendFontList 缓存，不立即下发
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_fontList_cache_beforeReady_001)
{
    TiptapChannelBridge bridge;
    EXPECT_FALSE(bridge.isEditorReady());

    QSignalSpy spy(&bridge, &TiptapChannelBridge::fontListProvided);
    QStringList fonts = {"Arial", "Noto Sans CJK SC"};
    bridge.sendFontList(fonts, "Noto Sans CJK SC");

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(bridge.pendingFontList(), fonts);
    EXPECT_EQ(bridge.pendingDefaultFont(), "Noto Sans CJK SC");
}

// 就绪后补发缓存的字体列表，不丢数据
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_fontList_replay_afterReady_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::fontListProvided);

    QStringList fonts = {"Arial", "Noto Sans CJK SC"};
    bridge.sendFontList(fonts, "Noto Sans CJK SC");
    EXPECT_EQ(spy.count(), 0);

    bridge.jsEditorReady();
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toStringList(), fonts);
    EXPECT_EQ(args.at(1).toString(), "Noto Sans CJK SC");
    EXPECT_TRUE(bridge.pendingFontList().isEmpty());
}

// 就绪后直接下发，不缓存
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_fontList_direct_afterReady_001)
{
    TiptapChannelBridge bridge;
    bridge.jsEditorReady();

    QSignalSpy spy(&bridge, &TiptapChannelBridge::fontListProvided);
    QStringList fonts = {"Arial"};
    bridge.sendFontList(fonts, "Arial");

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toStringList(), fonts);
    EXPECT_TRUE(bridge.pendingFontList().isEmpty());
}

// 单例访问返回同一实例
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_instance_returnsSame_001)
{
    TiptapChannelBridge *a = TiptapChannelBridge::instance();
    TiptapChannelBridge *b = TiptapChannelBridge::instance();
    EXPECT_NE(a, nullptr);
    EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// Voice 播放/转写入口（JS→C++）存在性与载荷
// ---------------------------------------------------------------------------

// jsRequestVoicePlayback 解析 voiceId，置 m_currentVoiceId，并 emit voicePlaybackRequested
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestVoicePlayback_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackRequested);
    QString info = "{\"voiceId\":\"voice-1\",\"voicePath\":\"voicenote/a.mp3\"}";
    bridge.jsRequestVoicePlayback(info);
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), info);
    EXPECT_FALSE(args.at(1).toBool());  // 首次播放 isSame=false
    EXPECT_EQ(bridge.currentVoiceId(), QStringLiteral("voice-1"));
}

// 同一 voiceId 再次请求播放 → isSame=true
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestVoicePlayback_isSame_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackRequested);
    QString info = "{\"voiceId\":\"voice-1\",\"voicePath\":\"voicenote/a.mp3\"}";
    bridge.jsRequestVoicePlayback(info);  // 首次
    bridge.jsRequestVoicePlayback(info);  // 同一 voiceId
    ASSERT_EQ(spy.count(), 2);
    auto secondArgs = spy.at(1);
    EXPECT_TRUE(secondArgs.at(1).toBool());  // isSame=true
}

// 切换到不同语音块时，先向旧 voiceId 派发 End 复位，再派发新块请求
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestVoicePlayback_switchResetsOld_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy reqSpy(&bridge, &TiptapChannelBridge::voicePlaybackRequested);
    QSignalSpy stateSpy(&bridge, &TiptapChannelBridge::voicePlaybackStateChanged);

    QString infoA = "{\"voiceId\":\"voice-a\",\"voicePath\":\"voicenote/a.mp3\"}";
    QString infoB = "{\"voiceId\":\"voice-b\",\"voicePath\":\"voicenote/b.mp3\"}";

    bridge.jsRequestVoicePlayback(infoA);  // 先播 A
    ASSERT_EQ(reqSpy.count(), 1);
    EXPECT_EQ(bridge.currentVoiceId(), QStringLiteral("voice-a"));
    EXPECT_EQ(stateSpy.count(), 0);  // 首次播放无旧块需复位

    bridge.jsRequestVoicePlayback(infoB);  // 再点 B
    // 先向旧 voiceId 派发 End 复位旧 NodeView
    ASSERT_EQ(stateSpy.count(), 1);
    auto stateArgs = stateSpy.takeFirst();
    EXPECT_EQ(stateArgs.at(0).toString(), QStringLiteral("voice-a"));
    EXPECT_EQ(stateArgs.at(1).toInt(), 2);  // 2 = End
    // 再派发新块播放请求
    ASSERT_EQ(reqSpy.count(), 2);
    EXPECT_FALSE(reqSpy.at(1).at(1).toBool());  // 切换不同块 isSame=false
    EXPECT_EQ(bridge.currentVoiceId(), QStringLiteral("voice-b"));
}

// jsRequestVoicePlaybackStop emit voicePlaybackStopRequested
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestVoicePlaybackStop_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackStopRequested);
    bridge.jsRequestVoicePlaybackStop();
    EXPECT_EQ(spy.count(), 1);
}

// jsRequestVoiceSeek emit voicePlaybackSeekRequested
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestVoiceSeek_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackSeekRequested);
    bridge.jsRequestVoiceSeek(QStringLiteral("5000"));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toLongLong(), 5000);
}

// jsRequestVoiceSeek 非法值不发信号
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestVoiceSeek_invalid_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackSeekRequested);
    bridge.jsRequestVoiceSeek(QStringLiteral("abc"));
    EXPECT_EQ(spy.count(), 0);
}

// jsRequestVoiceToText emit voiceToTextRequested
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsRequestVoiceToText_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voiceToTextRequested);
    QString info = "{\"voiceId\":\"voice-2\",\"voicePath\":\"voicenote/b.mp3\"}";
    bridge.jsRequestVoiceToText(info);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), info);
}

// ---------------------------------------------------------------------------
// Voice 播放/转写/主题信号下发（C++→JS）存在性与载荷
// ---------------------------------------------------------------------------

// emitVoicePlaybackStateChanged
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitVoicePlaybackStateChanged_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackStateChanged);
    bridge.emitVoicePlaybackStateChanged(QStringLiteral("v1"), 0);
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("v1"));
    EXPECT_EQ(args.at(1).toInt(), 0);
}

// emitVoicePlaybackPositionChanged
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitVoicePlaybackPositionChanged_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackPositionChanged);
    bridge.emitVoicePlaybackPositionChanged(QStringLiteral("v1"), 3000);
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("v1"));
    EXPECT_EQ(args.at(1).toLongLong(), 3000);
}

// emitVoicePlaybackDurationChanged
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitVoicePlaybackDurationChanged_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voicePlaybackDurationChanged);
    bridge.emitVoicePlaybackDurationChanged(QStringLiteral("v1"), 12000);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(1).toLongLong(), 12000);
}

// emitVoiceFileError
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitVoiceFileError_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voiceFileError);
    bridge.emitVoiceFileError(QStringLiteral("v1"));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("v1"));
}

// emitVoiceToTextStarted
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitVoiceToTextStarted_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voiceToTextStarted);
    bridge.emitVoiceToTextStarted(QStringLiteral("v1"));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("v1"));
}

// emitVoiceToTextFailed
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitVoiceToTextFailed_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voiceToTextFailed);
    bridge.emitVoiceToTextFailed(QStringLiteral("v1"));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("v1"));
}

// emitVoiceToTextCompleted
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitVoiceToTextCompleted_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::voiceToTextCompleted);
    bridge.emitVoiceToTextCompleted(QStringLiteral("v1"), QStringLiteral("转写结果"));
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("v1"));
    EXPECT_EQ(args.at(1).toString(), QStringLiteral("转写结果"));
}

// emitThemeProvided：编辑器 ready 后立即下发主题。
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitThemeProvided_001)
{
    TiptapChannelBridge bridge;
    bridge.jsEditorReady();

    QSignalSpy spy(&bridge, &TiptapChannelBridge::themeProvided);
    bridge.emitThemeProvided(QStringLiteral("dark"), QStringLiteral("#007AFF"),
                             QStringLiteral("#999999"), QStringLiteral("#090A17"));
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("dark"));
    EXPECT_EQ(args.at(1).toString(), QStringLiteral("#007AFF"));
}

// emitThemeProvided：ready 前缓存主题，ready 后补发，避免首次暗色主题信号丢失。
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_emitThemeProvided_cacheBeforeReady_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::themeProvided);

    bridge.emitThemeProvided(QStringLiteral("dark"), QStringLiteral("#007AFF"),
                             QStringLiteral("#999999"), QStringLiteral("#090A17"));
    EXPECT_EQ(spy.count(), 0);

    bridge.jsEditorReady();
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("dark"));
    EXPECT_EQ(args.at(1).toString(), QStringLiteral("#007AFF"));
    EXPECT_EQ(args.at(2).toString(), QStringLiteral("#999999"));
    EXPECT_EQ(args.at(3).toString(), QStringLiteral("#090A17"));
}

// currentVoiceId / setCurrentVoiceId
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_currentVoiceId_001)
{
    TiptapChannelBridge bridge;
    EXPECT_TRUE(bridge.currentVoiceId().isEmpty());
    bridge.setCurrentVoiceId(QStringLiteral("voice-x"));
    EXPECT_EQ(bridge.currentVoiceId(), QStringLiteral("voice-x"));
}

// ---------------------------------------------------------------------------
// 滚动位置上报（JS→C++）：jsReportScroll + scrollChanged 信号
// ---------------------------------------------------------------------------

// jsReportScroll(0) → scrollChanged(true)（已到顶部）
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsReportScroll_top_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::scrollChanged);
    bridge.jsReportScroll(0);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toBool(), true);
}

// jsReportScroll(正值) → scrollChanged(false)（未到顶部）
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_jsReportScroll_scrolled_001)
{
    TiptapChannelBridge bridge;
    QSignalSpy spy(&bridge, &TiptapChannelBridge::scrollChanged);
    bridge.jsReportScroll(150);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toBool(), false);
}
