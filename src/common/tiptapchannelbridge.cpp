// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tiptapchannelbridge.h"

#include <QResource>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QDateTime>
#include <QBuffer>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QHash>
#include <QUrl>

namespace {

// 复用 web_engine_handler.cpp 的图片路径归一化逻辑：将 file:// URL、
// images/ 相对路径或 WEB_PATH/images 绝对路径统一解析为 AppData/images 内的本地路径，
// 校验结果必须落在 AppData/images 目录内，否则返回空串（拒绝越界路径）。
QString normalizePicturePath(const QString &path)
{
    QString localPath = path;
    const QUrl url(path);
    if (url.isLocalFile()) {
        localPath = url.toLocalFile();
    }

    const QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(localPath));
    QString result;
    const QDir appDataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const QString appImageRoot = QDir::cleanPath(appDataDir.filePath(QStringLiteral("images")));
    const QString appImageRootWithSep = appImageRoot + QStringLiteral("/");

    if (normalized.startsWith(QStringLiteral("images/"))) {
        result = QDir::cleanPath(appDataDir.filePath(normalized));
    } else if (normalized.startsWith(appImageRootWithSep)) {
        result = normalized;
    } else {
        const QString webImageRoot = QDir::fromNativeSeparators(QStringLiteral(WEB_PATH) + QStringLiteral("/images/"));
        if (normalized.startsWith(webImageRoot)) {
            result = QDir::cleanPath(QDir(appImageRoot).filePath(normalized.mid(webImageRoot.size())));
        }
    }

    if (!result.startsWith(appImageRootWithSep)) {
        return QString();
    }
    return result;
}

} // namespace

TiptapChannelBridge::TiptapChannelBridge(QObject *parent)
    : QObject(parent)
    , m_editorReady(false)
    , m_pendingFontListValid(false)
    , m_imageSeq(0)
{
}

TiptapChannelBridge *TiptapChannelBridge::instance()
{
    static TiptapChannelBridge inst;
    return &inst;
}

bool TiptapChannelBridge::debugEnabled() const
{
    return !qgetenv("DVN_TIPTAP_DEBUG").isEmpty();
}

QString TiptapChannelBridge::tiptapHtmlPath() const
{
    // Tiptap needs to resolve AppData images through file:// URLs. Loading the
    // editor itself from file:// keeps the page in QtWebEngine's local-file path.
    const QString filePath = QStringLiteral(TIPTAP_WEB_PATH "/tiptap-editor.html");
    if (QFile::exists(filePath)) {
        return QUrl::fromLocalFile(filePath).toString();
    }
    return QStringLiteral("qrc:/tiptap-editor.html");
}

QString TiptapChannelBridge::resourceBaseUrl() const
{
    // 用户数据（图片/语音）存储在 AppDataLocation，前端用此根 + "images/xxx" 拼接绝对路径
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        return QString();
    }
    return QUrl::fromLocalFile(appDataPath).toString();
}

// ---------------------------------------------------------------------------
// 加载就绪缓存语义（R6）
// ---------------------------------------------------------------------------

void TiptapChannelBridge::notifyEditorReady()
{
    m_editorReady = true;
    emit editorReady();

    // 就绪后补发缓存的 envelope，不丢数据
    if (!m_pendingEnvelope.isEmpty()) {
        emit loadEnvelopeRequested(m_pendingEnvelope);
        m_pendingEnvelope.clear();
    }

    // 就绪后补发缓存的字体列表，不丢数据
    if (m_pendingFontListValid) {
        emit fontListProvided(m_pendingFontList, m_pendingDefaultFont);
        m_pendingFontList.clear();
        m_pendingDefaultFont.clear();
        m_pendingFontListValid = false;
    }

    // 工具栏在 QWebChannel 完成绑定前即可被点击；图片复制完成后若此时
    // 编辑器尚未 ready，不能直接 emit，否则 QWebChannel 信号会丢失。
    const QStringList pendingImages = m_pendingInsertImages;
    m_pendingInsertImages.clear();
    for (const QString &imageInfoJson : pendingImages) {
        emit insertImage(imageInfoJson);
    }
}

void TiptapChannelBridge::loadEnvelope(const QString &envelopeJson)
{
    if (m_editorReady) {
        // 已就绪，直接下发
        emit loadEnvelopeRequested(envelopeJson);
    } else {
        // 未就绪，缓存待发
        m_pendingEnvelope = envelopeJson;
    }
}

void TiptapChannelBridge::sendFontList(const QStringList &fonts, const QString &defaultFont)
{
    if (m_editorReady) {
        // 已就绪，直接下发
        emit fontListProvided(fonts, defaultFont);
    } else {
        // 未就绪，缓存待发
        m_pendingFontList = fonts;
        m_pendingDefaultFont = defaultFont;
        m_pendingFontListValid = true;
    }
}

QString TiptapChannelBridge::pendingEnvelope() const
{
    return m_pendingEnvelope;
}

QStringList TiptapChannelBridge::pendingFontList() const
{
    return m_pendingFontList;
}

QString TiptapChannelBridge::pendingDefaultFont() const
{
    return m_pendingDefaultFont;
}

bool TiptapChannelBridge::isEditorReady() const
{
    return m_editorReady;
}

// ---------------------------------------------------------------------------
// 保存往返
// ---------------------------------------------------------------------------

void TiptapChannelBridge::requestEditorContent()
{
    emit requestContent();
}

// ---------------------------------------------------------------------------
// 插入图片 / 语音
// ---------------------------------------------------------------------------

void TiptapChannelBridge::sendInsertImage(const QString &imageInfoJson)
{
    if (!m_editorReady) {
        m_pendingInsertImages.append(imageInfoJson);
        qInfo() << "Queueing image insertion until Tiptap editor is ready";
        return;
    }
    emit insertImage(imageInfoJson);
}

void TiptapChannelBridge::sendInsertVoiceBlock(const QString &voiceInfoJson)
{
    emit insertVoiceBlock(voiceInfoJson);
}

// ---------------------------------------------------------------------------
// JS→C++ 回告入口
// ---------------------------------------------------------------------------

void TiptapChannelBridge::jsEditorReady()
{
    qInfo() << "Tiptap editor ready (TiptapChannelBridge)";
    notifyEditorReady();
}

void TiptapChannelBridge::jsContentChanged()
{
    emit contentChanged();
}

void TiptapChannelBridge::jsContentSaved(const QString &envelopeJson)
{
    qInfo() << "Tiptap content saved, envelope length:" << envelopeJson.length();
    emit contentSaved(envelopeJson);
}

void TiptapChannelBridge::jsInsertImageFailed(const QString &reason)
{
    qWarning() << "Tiptap insert image failed:" << reason;
    emit insertImageFailed(reason);
}

void TiptapChannelBridge::jsInsertVoiceBlockFailed(const QString &reason)
{
    qWarning() << "Tiptap insert voice block failed:" << reason;
    emit insertVoiceBlockFailed(reason);
}

// ---------------------------------------------------------------------------
// 图片 UI 交互（不改动既有五类数据通道事件签名）
// ---------------------------------------------------------------------------

void TiptapChannelBridge::jsRequestPickImage()
{
    emit pickImageRequested();
}

void TiptapChannelBridge::jsRequestRecordVoice()
{
    emit recordVoiceRequested();
}

void TiptapChannelBridge::jsRequestViewPicture(const QString &url)
{
    const QString localPath = normalizePicturePath(url);
    if (localPath.isEmpty()) {
        qWarning() << "jsRequestViewPicture: refused unsafe image path:" << url;
        return;
    }
    emit viewPictureRequested(localPath);
}

void TiptapChannelBridge::jsPasteImage(const QString &dataUrl)
{
    // data URL 形如 data:image/png;base64,XXXX
    const int comma = dataUrl.indexOf(QLatin1Char(','));
    if (comma < 0) {
        qWarning() << "jsPasteImage: invalid data url";
        emit insertImageFailed("invalid pasted image data");
        return;
    }

    const QString meta = dataUrl.left(comma);
    const QByteArray base64 = dataUrl.mid(comma + 1).toUtf8();

    // 解析 mime 类型
    const int colon = meta.indexOf(QLatin1Char(':'));
    const int semi = meta.indexOf(QLatin1Char(';'));
    QString mime;
    if (colon >= 0 && semi > colon) {
        mime = meta.mid(colon + 1, semi - colon - 1);
    } else if (colon >= 0) {
        mime = meta.mid(colon + 1);
    }

    QString suffix;
    if (mime == QStringLiteral("image/png")) {
        suffix = QStringLiteral("png");
    } else if (mime == QStringLiteral("image/jpeg")) {
        suffix = QStringLiteral("jpg");
    } else if (mime == QStringLiteral("image/bmp")) {
        suffix = QStringLiteral("bmp");
    } else {
        qWarning() << "jsPasteImage: unsupported image format" << mime;
        emit insertImageFailed("unsupported pasted image format");
        return;
    }

    const QByteArray bytes = QByteArray::fromBase64(base64);
    QImage image;
    if (!image.loadFromData(bytes)) {
        qWarning() << "jsPasteImage: failed to decode image";
        emit insertImageFailed("failed to decode pasted image");
        return;
    }

    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + QStringLiteral("/images");
    QDir().mkdir(dirPath);
    const QString date = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
    const QString fileName = QString("%1_%2.%3").arg(date).arg(++m_imageSeq).arg(suffix);
    const QString imgPath = dirPath + QLatin1Char('/') + fileName;

    static const QHash<QString, QByteArray> formatBySuffix = {
        {QStringLiteral("png"), QByteArrayLiteral("PNG")},
        {QStringLiteral("jpg"), QByteArrayLiteral("JPEG")},
        {QStringLiteral("bmp"), QByteArrayLiteral("BMP")},
    };
    const QByteArray format = formatBySuffix.value(suffix);
    if (!image.save(imgPath, format.constData())) {
        qWarning() << "jsPasteImage: failed to save image to" << imgPath;
        emit insertImageFailed("failed to save pasted image");
        return;
    }

    QJsonObject info;
    info.insert(QStringLiteral("relPath"), QString(QStringLiteral("images/") + fileName));
    emit insertImage(QJsonDocument(info).toJson(QJsonDocument::Compact));
}

// ---------------------------------------------------------------------------
// 滚动位置上报（JS→C++）
// ---------------------------------------------------------------------------

void TiptapChannelBridge::jsReportScroll(int scrollTop)
{
    const bool isTop = (scrollTop <= 0);
    emit scrollChanged(isTop);
}


// ---------------------------------------------------------------------------
// Voice 播放/转写入口（JS→C++）
// ---------------------------------------------------------------------------

void TiptapChannelBridge::jsRequestVoicePlayback(const QString &voiceInfoJson)
{
    QJsonDocument doc = QJsonDocument::fromJson(voiceInfoJson.toUtf8());
    QJsonObject obj = doc.object();
    QString voiceId = obj.value(QStringLiteral("voiceId")).toString();
    bool isSame = (voiceId == m_currentVoiceId);
    // 切换到不同语音块前，先向旧 voiceId 派发结束状态，让旧 NodeView 复位为非播放态。
    // 2 = End（与 VoicePlayerHandler::PlayState::End 对齐，前端 NodeView 据此重置）。
    if (!isSame && !m_currentVoiceId.isEmpty()) {
        emit voicePlaybackStateChanged(m_currentVoiceId, 2);
    }
    m_currentVoiceId = voiceId;
    emit voicePlaybackRequested(voiceInfoJson, isSame);
}

void TiptapChannelBridge::jsRequestVoicePlaybackStop()
{
    emit voicePlaybackStopRequested();
}

void TiptapChannelBridge::jsRequestVoiceSeek(const QString &ms)
{
    bool ok = false;
    qint64 pos = ms.toLongLong(&ok);
    if (ok) {
        emit voicePlaybackSeekRequested(pos);
    }
}

void TiptapChannelBridge::jsRequestVoiceToText(const QString &voiceInfoJson)
{
    emit voiceToTextRequested(voiceInfoJson);
}

// ---------------------------------------------------------------------------
// Voice 播放/转写/主题信号下发（C++→JS）
// ---------------------------------------------------------------------------

void TiptapChannelBridge::emitVoicePlaybackStateChanged(const QString &voiceId, int state)
{
    emit voicePlaybackStateChanged(voiceId, state);
}

void TiptapChannelBridge::emitVoicePlaybackPositionChanged(const QString &voiceId, qint64 ms)
{
    emit voicePlaybackPositionChanged(voiceId, ms);
}

void TiptapChannelBridge::emitVoicePlaybackDurationChanged(const QString &voiceId, qint64 ms)
{
    emit voicePlaybackDurationChanged(voiceId, ms);
}

void TiptapChannelBridge::emitVoiceFileError(const QString &voiceId)
{
    emit voiceFileError(voiceId);
}

void TiptapChannelBridge::emitVoiceToTextStarted(const QString &voiceId)
{
    emit voiceToTextStarted(voiceId);
}

void TiptapChannelBridge::emitVoiceToTextFailed(const QString &voiceId)
{
    emit voiceToTextFailed(voiceId);
}

void TiptapChannelBridge::emitVoiceToTextCompleted(const QString &voiceId, const QString &text)
{
    emit voiceToTextCompleted(voiceId, text);
}

void TiptapChannelBridge::emitThemeProvided(const QString &theme, const QString &highlightColor,
                                             const QString &disableHighlightColor,
                                             const QString &backgroundColor)
{
    emit themeProvided(theme, highlightColor, disableHighlightColor, backgroundColor);
}

QString TiptapChannelBridge::currentVoiceId() const
{
    return m_currentVoiceId;
}

void TiptapChannelBridge::setCurrentVoiceId(const QString &voiceId)
{
    m_currentVoiceId = voiceId;
}
