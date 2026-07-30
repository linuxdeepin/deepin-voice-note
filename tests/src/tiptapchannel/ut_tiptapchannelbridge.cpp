// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_tiptapchannelbridge.h"
#include "tiptapchannelbridge.h"

#include <QResource>
#include <QSignalSpy>
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

// tiptapHtmlPath 返回 qrc 或 file:// 路径
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_tiptapHtmlPath_001)
{
    TiptapChannelBridge bridge;
    QString path = bridge.tiptapHtmlPath();
    if (QResource(":/tiptap-editor.html").isValid()) {
        EXPECT_TRUE(path.startsWith("qrc:")) << path.toStdString();
    } else {
        EXPECT_TRUE(path.startsWith("file://")) << path.toStdString();
    }
    EXPECT_TRUE(path.contains("tiptap-editor.html")) << path.toStdString();
}

// debugEnabled 环境变量门控
TEST_F(UT_TiptapChannelBridge, UT_TiptapChannelBridge_debugEnabled_001)
{
    qputenv("DVN_TIPTAP_DEBUG", "");
    TiptapChannelBridge bridge1;
    EXPECT_FALSE(bridge1.debugEnabled());

    qputenv("DVN_TIPTAP_DEBUG", "1");
    TiptapChannelBridge bridge2;
    EXPECT_TRUE(bridge2.debugEnabled());

    qputenv("DVN_TIPTAP_DEBUG", "");
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
