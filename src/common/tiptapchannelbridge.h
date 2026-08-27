// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIPTAPCHANNELBRIDGE_H
#define TIPTAPCHANNELBRIDGE_H

#include <QObject>
#include <QStringList>

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
// 附加只增事件：字体列表下发（C++→JS，编辑器就绪后补发）
//   - signal fontListProvided(fonts, defaultFont)         （C++→JS 下发）
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
    Q_PROPERTY(int currentNoteId READ currentNoteId NOTIFY currentNoteChanged)
    Q_PROPERTY(QString currentNoteTitle READ currentNoteTitle NOTIFY currentNoteChanged)

public:
    explicit TiptapChannelBridge(QObject *parent = nullptr);

    // 全局单例访问（与 QML 单例共享同一实例，供宿主侧接线）
    static TiptapChannelBridge *instance();

    // --- 适配层方法（Q_INVOKABLE，供 QML / 前端调用） ---

    // 读 DVN_TIPTAP_DEBUG 环境变量，便于 QA 与 GTest
    Q_INVOKABLE bool debugEnabled() const;

    // 返回 Tiptap 运行时 HTML 路径（首选 file://，资源缺失时回退 qrc）
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

    // 前端请求宿主打开图片选择对话框（工具栏图片按钮）
    Q_INVOKABLE void jsRequestPickImage();

    // 前端请求开始录音（工具栏麦克风按钮）
    Q_INVOKABLE void jsRequestRecordVoice();

    // 工作区标题读写（标题显示在编辑器页面中，但不属于正文文档）
    Q_INVOKABLE int currentNoteId() const;
    Q_INVOKABLE QString currentNoteTitle() const;
    Q_INVOKABLE void renameCurrentNote(const QString &title);

    // 前端请求查看原图（双击图片），宿主归一化路径后下发预览
    Q_INVOKABLE void jsRequestViewPicture(const QString &url);

    // --- voice 播放/转写入口（前端 JS → C++） ---
    // 前端请求播放语音，voiceInfoJson 含 voiceId/voicePath(相对)/voiceSize/title/createTime
    Q_INVOKABLE void jsRequestVoicePlayback(const QString &voiceInfoJson);
    // 前端请求停止播放
    Q_INVOKABLE void jsRequestVoicePlaybackStop();
    // 前端请求跳转播放进度（毫秒）
    Q_INVOKABLE void jsRequestVoiceSeek(const QString &ms);
    // 前端请求语音转文字
    Q_INVOKABLE void jsRequestVoiceToText(const QString &voiceInfoJson);

    // 前端粘贴剪贴板图片数据（data URL），宿主保存到 images/ 后回插
    Q_INVOKABLE void jsPasteImage(const QString &dataUrl);

    // 前端上报编辑器滚动位置（scrollTop），宿主据此驱动标题栏阴影状态
    Q_INVOKABLE void jsReportScroll(int scrollTop);

    // 宿主侧下发字体列表（未就绪时缓存，就绪后补发）
    Q_INVOKABLE void sendFontList(const QStringList &fonts, const QString &defaultFont);

    // --- voice 播放/转写信号下发（C++ → JS，由宿主调用） ---
    void emitVoicePlaybackStateChanged(const QString &voiceId, int state);
    void emitVoicePlaybackPositionChanged(const QString &voiceId, qint64 ms);
    void emitVoicePlaybackDurationChanged(const QString &voiceId, qint64 ms);
    void emitVoiceFileError(const QString &voiceId);
    void emitVoiceToTextStarted(const QString &voiceId);
    void emitVoiceToTextFailed(const QString &voiceId);
    void emitVoiceToTextCompleted(const QString &voiceId, const QString &text);
    // 主题下发（深浅色 + 高亮色），前端写 CSS 变量
    void emitThemeProvided(const QString &theme, const QString &highlightColor,
                           const QString &disableHighlightColor, const QString &backgroundColor);

    // 测试辅助：获取当前缓存的待下发 envelope（未就绪时）
    Q_INVOKABLE QString pendingEnvelope() const;

    // 测试辅助：获取当前缓存的待下发字体列表与默认字体（未就绪时）
    Q_INVOKABLE QStringList pendingFontList() const;
    Q_INVOKABLE QString pendingDefaultFont() const;

    // 测试辅助：是否已就绪
    Q_INVOKABLE bool isEditorReady() const;

    // voice 运行态：当前播放的语音块唯一标识
    QString currentVoiceId() const;
    void setCurrentVoiceId(const QString &voiceId);

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

    // C++→JS：字体列表下发（供工具栏字体下拉填充）
    void fontListProvided(const QStringList &fonts, const QString &defaultFont);

    // C++→JS：voice 播放状态变化（0=播放, 1=暂停, 2=结束）
    void voicePlaybackStateChanged(const QString &voiceId, int state);
    // C++→JS：voice 播放进度变化
    void voicePlaybackPositionChanged(const QString &voiceId, qint64 ms);
    // C++→JS：voice 时长变化
    void voicePlaybackDurationChanged(const QString &voiceId, qint64 ms);
    // C++→JS：voice 文件不可播放（缺失或损坏）
    void voiceFileError(const QString &voiceId);
    // C++→JS：voice 转文字开始
    void voiceToTextStarted(const QString &voiceId);
    // C++→JS：voice 转文字失败
    void voiceToTextFailed(const QString &voiceId);
    // C++→JS：voice 转文字完成
    void voiceToTextCompleted(const QString &voiceId, const QString &text);
    // C++→JS：主题下发
    void themeProvided(const QString &theme, const QString &highlightColor,
                       const QString &disableHighlightColor, const QString &backgroundColor);
    // C++→JS：当前笔记标题变化
    void currentNoteChanged(int noteId, const QString &title);

    // JS→C++：滚动位置上报（isTop=true 表示已滚到顶部）
    void scrollChanged(bool isTop);

    // C++ 内部请求信号（WebEngineHandler 连接处理）
    void voicePlaybackRequested(const QString &voiceInfoJson, bool isSame);
    void voicePlaybackStopRequested();
    void voicePlaybackSeekRequested(qint64 ms);
    void voiceToTextRequested(const QString &voiceInfoJson);

    // C++→QML：前端请求打开图片选择对话框
    void pickImageRequested();

    // C++→QML：前端请求开始录音
    void recordVoiceRequested();

    // C++→QML：前端请求查看原图，携带归一化后的本地路径
    void viewPictureRequested(const QString &localPath);

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
    QStringList m_pendingInsertImages;
    QStringList m_pendingFontList;
    QString m_pendingDefaultFont;
    bool m_pendingFontListValid;
    int m_imageSeq;
    QString m_currentVoiceId;
};

#endif // TIPTAPCHANNELBRIDGE_H
