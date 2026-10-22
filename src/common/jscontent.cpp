// Copyright (C) 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "jscontent.h"
#include "ddlog.h"

#include <QFile>
#include <QVariant>
#include <QEventLoop>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QClipboard>
#include <QMimeData>
#include <QDateTime>
#include <QImage>
#include <QDebug>
#include <QApplication>
// 条件编译：Qt5 需要包含 functional 头文件
#ifdef USE_QT5
#include <functional>
#endif

JsContent::JsContent()
{
    qCInfo(voiceNoteLog) << "JsContent constructor called";
    connect(QApplication::clipboard(), &QClipboard::changed, this, &JsContent::onClipChange);
    qCInfo(voiceNoteLog) << "JsContent constructor finished";
}

JsContent *JsContent::instance()
{
    // qInfo() << "JsContent instance requested";
    static JsContent _instance;
    return &_instance;
}

/**
 * @brief JsContent::insertImages
 * 判断图片路径是否有效，存在有效路径则创建副本传到web端中
 * @param filePaths 图片路径
 * @return 此次操作是否有效
 */
bool JsContent::insertImages(QStringList filePaths)
{
    qCDebug(voiceNoteLog) << "Attempting to insert" << filePaths.size() << "images";
    int count = 0;
    QStringList paths;
    // 获取文件夹路径
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/images";
    // 创建文件夹
    QDir().mkdir(dirPath);
    // 获取时间戳
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString date = currentDateTime.toString("yyyyMMddhhmmss");

    for (auto path : filePaths) {
        QFileInfo fileInfo(path);
        QString suffix = fileInfo.suffix();
        if (!(suffix == "jpg" || suffix == "png" || suffix == "bmp")) {
            qCWarning(voiceNoteLog) << "Skipping unsupported image format:" << suffix << "from path:" << path;
            continue;
        }
        // 创建文件路径
        QString newPath = QString("%1/%2_%3.%4").arg(dirPath).arg(date).arg(++count).arg(suffix);
        if (QFile::copy(path, newPath)) {
            QString fileName = QString("%1_%2.%3").arg(date).arg(count).arg(suffix);
            paths.push_back(QString("images/") + fileName);
            qCDebug(voiceNoteLog) << "Successfully copied image to:" << newPath << "sending relative path:" << paths.last();
        } else {
            qCWarning(voiceNoteLog) << "Failed to copy image from:" << path << "to:" << newPath;
        }
    }
    if (paths.size() == 0) {
        qCWarning(voiceNoteLog) << "No valid images were processed";
        return false;
    }
    qCDebug(voiceNoteLog) << "Emitting callJsInsertImages with" << paths.size() << "images";
    emit callJsInsertImages(paths);
    qCInfo(voiceNoteLog) << "Image insertion finished";
    return true;
}

/**
 * @brief JsContent::insertImages
 * 向web端传入图片
 * @param image
 * @return 操作是否成功 true:成功
 */
bool JsContent::insertImages(const QImage &image)
{
    qCDebug(voiceNoteLog) << "Attempting to insert single image";
    // 获取文件夹路径
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/images";
    // 创建文件夹
    QDir().mkdir(dirPath);
    // 保存文件，文件名为当前年月日时分秒
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString fileName = currentDateTime.toString("yyyyMMddhhmmss.png");
    QString imgPath = dirPath + "/" + fileName;

    // 保存文件
    if (!image.save(imgPath)) {
        qCWarning(voiceNoteLog) << "Failed to save image to:" << imgPath;
        return false;
    }
    qCDebug(voiceNoteLog) << "Successfully saved image to:" << imgPath;
    // 转换为相对路径发送给前端（images/xxx）
    emit callJsInsertImages(QStringList(QString("images/") + fileName));
    qCInfo(voiceNoteLog) << "Single image insertion finished";
    return true;
}

QString JsContent::webPath()
{
    qCInfo(voiceNoteLog) << "Getting web path";
    QString path = "file://" WEB_PATH "/index.html";
    qCInfo(voiceNoteLog) << "Web path retrieved";
    return path;
}

QString JsContent::jsCallGetAppDataPath()
{
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        qCWarning(voiceNoteLog) << "Provide AppData base path to JS failed: empty AppData path";
        return QString();
    }
    QDir dir(appDataPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCWarning(voiceNoteLog) << "Provide AppData base path to JS failed: mkpath failed for" << appDataPath;
            return QString();
        }
    }
    QString basePath = QDir::toNativeSeparators(dir.absolutePath());
    if (!basePath.endsWith(QDir::separator())) {
        basePath += QDir::separator();
    }
    QUrl url = QUrl::fromLocalFile(basePath);
    const QString urlStr = url.toString();
    qCInfo(voiceNoteLog) << "Provide AppData base path to JS:" << urlStr;
    return urlStr;
}

void JsContent::jsCallTxtChange()
{
    qCInfo(voiceNoteLog) << "Text change called from JavaScript";
    emit textChange();
    qCInfo(voiceNoteLog) << "Text change handling finished";
}

void JsContent::jsCallChannleFinish()
{
    qCInfo(voiceNoteLog) << "Channel finish called from JavaScript";
    emit getfontinfo();
    qCInfo(voiceNoteLog) << "Channel finish handling finished";
}

void JsContent::jsCallSummernoteInitFinish()
{
    qCInfo(voiceNoteLog) << "Summernote init finish called from JavaScript";
    emit loadFinsh();
    qCInfo(voiceNoteLog) << "Summernote init finish handling finished";
}

void JsContent::jsCallPopupMenu(int type, const QVariant &json)
{
    qCInfo(voiceNoteLog) << "Popup menu called from JavaScript, type:" << type;
    emit popupMenu(type, json);
    qCInfo(voiceNoteLog) << "Popup menu handling finished";
}

void JsContent::jsCallPlayVoice(const QVariant &json, bool bIsSame)
{
    qCInfo(voiceNoteLog) << "Play voice called from JavaScript, isSame:" << bIsSame;
    emit playVoice(json, bIsSame);
    qCInfo(voiceNoteLog) << "Play voice handling finished";
}

QString JsContent::jsCallGetTranslation()
{
    qCInfo(voiceNoteLog) << "Translation data requested from JavaScript";
    qCDebug(voiceNoteLog) << "Translation data requested from JavaScript";
    static QJsonDocument doc;
    if (doc.isEmpty()) {
        QJsonObject object;
        object.insert("fontname", QApplication::translate("web", "Font"));
        object.insert("fontsize", QApplication::translate("web", "Font size"));
        object.insert("forecolor", QApplication::translate("web", "Font color"));
        object.insert("backcolor", QApplication::translate("web", "Text highlight color"));
        object.insert("bold", QApplication::translate("web", "Bold"));
        object.insert("italic", QApplication::translate("web", "Italic"));
        object.insert("underline", QApplication::translate("web", "Underline"));
        object.insert("strikethrough", QApplication::translate("web", "Strikethrough"));
        object.insert("ul", QApplication::translate("web", "Bullets"));
        object.insert("ol", QApplication::translate("web", "Numbering"));
        object.insert("more", QApplication::translate("web", "More colors"));
        object.insert("recentlyUsed", QApplication::translate("web", "Recent"));
        doc.setObject(object);
        qCDebug(voiceNoteLog) << "Translation data initialized";
    }
    QString result = doc.toJson(QJsonDocument::Compact);
    qCInfo(voiceNoteLog) << "Translation data retrieval finished";
    return result;
}

/**
   @return Js前端调用返回部分 HTML DOM 节点展示需要的翻译文本
 */
QString JsContent::jsCallDivTextTranslation()
{
    qCDebug(voiceNoteLog) << "Div text translation requested from JavaScript";
    static QJsonDocument doc;
    if (doc.isEmpty()) {
        QJsonObject object;
        object.insert("translateLabel", QApplication::translate("web", "Voice To Text"));
        object.insert("translatingLabel", QApplication::translate("web", "Converting voice to text"));
        doc.setObject(object);
        qCDebug(voiceNoteLog) << "Div text translation data initialized";
    }
    return doc.toJson(QJsonDocument::Compact);
}

void JsContent::jsCallScrollChange(int scrollTop)
{
    qCInfo(voiceNoteLog) << "Scroll change called from JavaScript, scrollTop:" << scrollTop;
    emit scrollTopChange(scrollTop == 0);
    qCInfo(voiceNoteLog) << "Scroll change handling finished";
}

/**
   @brief Js前端调用停止播放
 */
void JsContent::jsCallPlayVoiceStop()
{
    qCInfo(voiceNoteLog) << "Play voice stop called from JavaScript";
    Q_EMIT playVoiceStop();
    qCInfo(voiceNoteLog) << "Play voice stop handling finished";
}

/**
   @brief 变更播放进度，仅在手动拖拽进度条时触发
   @param progressMs 拖拽后的进度毫秒(ms)值
 */
void JsContent::jsCallVoiceProgressChange(qint64 progressMs)
{
    qCInfo(voiceNoteLog) << "Voice progress change called from JavaScript, progressMs:" << progressMs;
    Q_EMIT playVoiceProgressChange(progressMs);
    qCInfo(voiceNoteLog) << "Voice progress change handling finished";
}

QVariant JsContent::callJsSynchronous(QWebEnginePage *page, const QString &funtion)
{
    qCDebug(voiceNoteLog) << "Executing synchronous JavaScript function";
    QVariant synResult;
    QEventLoop synLoop;
    if (page) {
#ifdef USE_QT5
        // Qt5 中使用 std::function 作为回调
        std::function<void(const QVariant&)> callback = [&synResult, &synLoop](const QVariant &result) {
            synResult = result;
            synLoop.quit();
        };
        page->runJavaScript(funtion, callback);
#else
        // Qt6 中使用 QWebEngineCallback
        page->runJavaScript(funtion, [&](const QVariant &result) {
            synResult = result;
            synLoop.quit();
        });
#endif
        synLoop.exec();
        qCDebug(voiceNoteLog) << "Synchronous JavaScript execution completed";
    } else {
        qCWarning(voiceNoteLog) << "Attempted to execute JavaScript on null page";
    }
    qCInfo(voiceNoteLog) << "Synchronous JavaScript execution finished";
    return synResult;
}

void JsContent::jsCallSetDataFinsh()
{
    qCInfo(voiceNoteLog) << "Set data finish called from JavaScript";
    emit setDataFinsh();
    qCInfo(voiceNoteLog) << "Set data finish handling finished";
}

void JsContent::jsCallPaste(bool isVoicePaste)
{
    qCInfo(voiceNoteLog) << "Paste called from JavaScript, isVoicePaste:" << isVoicePaste;
    emit textPaste(isVoicePaste);
    qCInfo(voiceNoteLog) << "Paste handling finished";
}

void JsContent::jsCallViewPicture(const QString &imagePath)
{
    qCInfo(voiceNoteLog) << "View picture called from JavaScript, imagePath:" << imagePath;
    emit viewPictrue(imagePath);
    qCInfo(voiceNoteLog) << "View picture handling finished";
}

void JsContent::jsCallCreateNote()
{
    qCInfo(voiceNoteLog) << "Create note called from JavaScript";
    emit createNote();
    qCInfo(voiceNoteLog) << "Create note handling finished";
}

void JsContent::jsCallSaveAudio()
{
    qCInfo(voiceNoteLog) << "Save audio called from JavaScript";
    emit saveAudio();
    qCInfo(voiceNoteLog) << "Save audio handling finished";
}

void JsContent::jsCallSetClipData(const QString &text, const QString &html)
{
    qCDebug(voiceNoteLog) << "Setting clipboard data from JavaScript";
    QClipboard *clip = QApplication::clipboard();
    if (nullptr != clip) {
        // 剪切板先断开与前端的通信
        clip->disconnect(this);
        // 更新剪切板
        QMimeData *data = new QMimeData;
        // 设置纯文本
        data->setText(text);
        // 设置富文本
        data->setHtml(html);
        clip->setMimeData(data);
        m_clipData = data;
        // 剪切板重新建立与前端的通信
        connect(QApplication::clipboard(), &QClipboard::changed, this, &JsContent::onClipChange);
        qCDebug(voiceNoteLog) << "Clipboard data updated successfully";
    } else {
        qCWarning(voiceNoteLog) << "Failed to access clipboard";
    }
    qCInfo(voiceNoteLog) << "Clipboard data setting finished";
}

void JsContent::onClipChange(QClipboard::Mode mode)
{
    qCDebug(voiceNoteLog) << "Clipboard change detected, mode:" << mode;
    if (QClipboard::Clipboard == mode && QApplication::clipboard()->mimeData() != m_clipData) {
        qCDebug(voiceNoteLog) << "Clipboard content changed externally";
        emit callJsClipboardDataChanged();
    }
    qCDebug(voiceNoteLog) << "Clipboard change handling finished";
}
