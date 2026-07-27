// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_tiptaptempbridge.h"
#include "tiptaptempbridge.h"

#include <QResource>
#include <QByteArray>

UT_TiptapTempBridge::UT_TiptapTempBridge()
{
}

// debugEnabled() 环境变量未设时返回 false
TEST_F(UT_TiptapTempBridge, UT_TiptapTempBridge_debugEnabled_default_001)
{
    // 确保环境变量未设置
    qputenv("DVN_TIPTAP_DEBUG", "");
    TiptapTempBridge bridge;
    EXPECT_FALSE(bridge.debugEnabled());
}

// debugEnabled() 环境变量设置时返回 true
TEST_F(UT_TiptapTempBridge, UT_TiptapTempBridge_debugEnabled_set_001)
{
    qputenv("DVN_TIPTAP_DEBUG", "1");
    TiptapTempBridge bridge;
    EXPECT_TRUE(bridge.debugEnabled());
    // 清理
    qputenv("DVN_TIPTAP_DEBUG", "");
}

// tiptapHtmlPath() 返回 qrc 路径（当 qrc 资源存在时）
TEST_F(UT_TiptapTempBridge, UT_TiptapTempBridge_tiptapHtmlPath_001)
{
    TiptapTempBridge bridge;
    QString path = bridge.tiptapHtmlPath();

    // 当 qrc 资源存在时应返回 qrc 路径，否则回退 file:// 路径
    if (QResource(":/tiptap-editor.html").isValid()) {
        EXPECT_TRUE(path.startsWith("qrc:")) << path.toStdString();
        EXPECT_TRUE(path.contains("tiptap-editor.html")) << path.toStdString();
    } else {
        EXPECT_TRUE(path.startsWith("file://")) << path.toStdString();
        EXPECT_TRUE(path.contains("tiptap-editor.html")) << path.toStdString();
    }
}

// QResource 资源注册冒烟测试
TEST_F(UT_TiptapTempBridge, UT_TiptapTempBridge_resourceExists_001)
{
    // 此测试需要构建产物（QRC 编译进二进制），CI 环境运行
    bool exists = QResource(":/tiptap-editor.html").isValid();
    EXPECT_TRUE(exists) << "QResource :/tiptap-editor.html not found — "
                           "ensure deepin-voice-note-tiptap.qrc is compiled in";
}

// jsEditorReady / jsContentSaved 可安全调用（无崩溃）
TEST_F(UT_TiptapTempBridge, UT_TiptapTempBridge_jsCallbacks_001)
{
    TiptapTempBridge bridge;
    bridge.jsEditorReady();
    bridge.jsContentSaved("{\"format\":\"tiptap\",\"schemaVersion\":1,\"content\":{\"type\":\"doc\",\"content\":[{\"type\":\"paragraph\"}]}}");
    SUCCEED();
}
