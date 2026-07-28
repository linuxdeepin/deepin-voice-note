// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIPTAPCHANNELBRIDGE_H
#define TIPTAPCHANNELBRIDGE_H

#include <QObject>

// ============================================================================
// 命名稳定契约：TiptapChannelBridge
// ============================================================================
// 本类是 C++ 宿主 ↔ Tiptap 前端之间的正式 QWebChannel 接口对象。
// QML 单例名：TiptapChannel，QWebChannel 注册名："tiptapChannel"。
//
// 命名一经确定即稳定，后续能力迁移只增不改、不重命名。
// 当前锁定五类基础事件：
//   1. 加载就绪   — signal editorReady()              （JS→C++）
//   2. 内容变化   — signal contentChanged()           （JS→C++，节流后回告）
//   3. 保存       — signal requestContent()           （C++→JS 请求）
//                  signal contentSaved(envelopeJson)  （JS→C++ 回告）
//   4. 插入图片   — signal insertImage(imageInfoJson) （C++→JS 下发）
//                  signal insertImageFailed(reason)   （JS→C++ 失败回告）
//   5. 插入语音   — signal insertVoiceBlock(voiceInfoJson)（C++→JS 下发）
//                  signal insertVoiceBlockFailed(reason)  （JS→C++ 失败回告）
//
// 适配层方法衔接宿主既有资源路径（WEB_PATH / images/ 相对路径约定），
// 只读复用 jscontent.cpp 路径模式，不修改 JsContent 源码。
// ============================================================================

class TiptapChannelBridge : public QObject
{
    Q_OBJECT

    // 适配层属性：宿主资源根路径（供前端拼接 images/ 等相对路径）
    Q_PROPERTY(QString resourceBaseUrl READ resourceBaseUrl CONSTANT)
    // debug 门控
    Q_PROPERTY(bool debugEnabled READ debugEnabled CONSTANT)

public:
    explicit TiptapChannelBridge(QObject *parent = nullptr);

    // --- 适配层方法（Q_INVOKABLE，供 QML / 前端调用） ---

    // 读 DVN_TIPTAP_DEBUG 环境变量，便于 QA 与 GTest
    Q_INVOKABLE bool debugEnabled() const;

    // 返回 Tiptap 运行时 HTML 路径（首选 qrc，install 回退 file://）
    Q_INVOKABLE QString tiptapHtmlPath() const;

    // 返回宿主资源根 URL（file:// + WEB_PATH 或 TIPTAP_WEB_PATH），
    // 前端用此根 + "images/xxx" 拼接图片绝对路径
    Q_INVOKABLE QString resourceBaseUrl() const;

    // --- 加载就绪缓存语义（R6） ---
    // 前端就绪后调用；宿主收到后才下发缓存请求，不丢数据
    Q_INVOKABLE void notifyEditorReady();

    // 宿主侧请求前端返回当前内容（emit requestContent）
    Q_INVOKABLE void requestEditorContent();

    // 宿主侧下发加载 envelope（未就绪时缓存，就绪后补发）
    Q_INVOKABLE void loadEnvelope(const QString &envelopeJson);

    // 宿主侧下发插入图片
    Q_INVOKABLE void sendInsertImage(const QString &imageInfoJson);

    // 宿主侧下发插入语音块
    Q_INVOKABLE void sendInsertVoiceBlock(const QString &voiceInfoJson);

    // 测试辅助：获取当前缓存的待下发 envelope（未就绪时）
    Q_INVOKABLE QString pendingEnvelope() const;

    // 测试辅助：是否已就绪
    Q_INVOKABLE bool isEditorReady() const;

signals:
    // --- 事件 1：加载就绪（JS→C++） ---
    void editorReady();

    // --- 事件 2：内容变化（JS→C++，节流后回告，不携带摘要） ---
    void contentChanged();

    // --- 事件 3：保存 ---
    // C++→JS：宿主请求前端返回内容
    void requestContent();
    // JS→C++：前端回告保存 envelope
    void contentSaved(const QString &envelopeJson);

    // --- 事件 4：插入图片 ---
    // C++→JS：宿主下发图片信息
    void insertImage(const QString &imageInfoJson);
    // JS→C++：前端插入失败回告
    void insertImageFailed(const QString &reason);

    // --- 事件 5：插入语音 ---
    // C++→JS：宿主下发语音块信息
    void insertVoiceBlock(const QString &voiceInfoJson);
    // JS→C++：前端插入失败回告
    void insertVoiceBlockFailed(const QString &reason);

    // C++→JS：加载 envelope（宿主下发内容到前端）
    void loadEnvelopeRequested(const QString &envelopeJson);

public slots:
    // JS→C++ 回告入口（Q_INVOKABLE 供前端直接调用）
    // 前端编辑器初始化完成
    Q_INVOKABLE void jsEditorReady();
    // 前端内容变化通知
    Q_INVOKABLE void jsContentChanged();
    // 前端保存 envelope 回传
    Q_INVOKABLE void jsContentSaved(const QString &envelopeJson);
    // 前端插入图片失败回告
    Q_INVOKABLE void jsInsertImageFailed(const QString &reason);
    // 前端插入语音块失败回告
    Q_INVOKABLE void jsInsertVoiceBlockFailed(const QString &reason);

private:
    bool m_editorReady;
    QString m_pendingEnvelope;
};

#endif // TIPTAPCHANNELBRIDGE_H
