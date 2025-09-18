// Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "jscontent.h"

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
    qInfo() << "JsContent constructor called";
    connect(QApplication::clipboard(), &QClipboard::changed, this, &JsContent::onClipChange);
    qInfo() << "JsContent constructor finished";
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
    qDebug() << "Attempting to insert" << filePaths.size() << "images";
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
            qWarning() << "Skipping unsupported image format:" << suffix << "from path:" << path;
            continue;
        }
        // 创建文件路径
        QString newPath = QString("%1/%2_%3.%4").arg(dirPath).arg(date).arg(++count).arg(suffix);
        if (QFile::copy(path, newPath)) {
            paths.push_back(newPath);
            qDebug() << "Successfully copied image to:" << newPath;
        } else {
            qWarning() << "Failed to copy image from:" << path << "to:" << newPath;
        }
    }
    if (paths.size() == 0) {
        qWarning() << "No valid images were processed";
        return false;
    }
    qDebug() << "Emitting callJsInsertImages with" << paths.size() << "images";
    emit callJsInsertImages(paths);
    qInfo() << "Image insertion finished";
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
    qDebug() << "Attempting to insert single image";
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
        qWarning() << "Failed to save image to:" << imgPath;
        return false;
    }
    qDebug() << "Successfully saved image to:" << imgPath;
    emit callJsInsertImages(QStringList(imgPath));
    qInfo() << "Single image insertion finished";
    return true;
}

QString JsContent::webPath()
{
    qInfo() << "Getting web path";
    QString path = "file://" WEB_PATH "/index.html";
    qInfo() << "Web path retrieved";
    return path;
}

void JsContent::jsCallTxtChange()
{
    qInfo() << "Text change called from JavaScript";
    emit textChange();
    qInfo() << "Text change handling finished";
}

void JsContent::jsCallChannleFinish()
{
    qInfo() << "Channel finish called from JavaScript";
    emit getfontinfo();
    qInfo() << "Channel finish handling finished";
}

void JsContent::jsCallSummernoteInitFinish()
{
    qInfo() << "Summernote init finish called from JavaScript";
    emit loadFinsh();
    qInfo() << "Summernote init finish handling finished";
}

void JsContent::jsCallPopupMenu(int type, const QVariant &json)
{
    qInfo() << "Popup menu called from JavaScript, type:" << type;
    emit popupMenu(type, json);
    qInfo() << "Popup menu handling finished";
}

void JsContent::jsCallPlayVoice(const QVariant &json, bool bIsSame)
{
    qInfo() << "Play voice called from JavaScript, isSame:" << bIsSame;
    emit playVoice(json, bIsSame);
    qInfo() << "Play voice handling finished";
}

QString JsContent::jsCallGetTranslation()
{
    qInfo() << "Translation data requested from JavaScript";
    qDebug() << "Translation data requested from JavaScript";
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
        qDebug() << "Translation data initialized";
    }
    QString result = doc.toJson(QJsonDocument::Compact);
    qInfo() << "Translation data retrieval finished";
    return result;
}

/**
   @return Js前端调用返回部分 HTML DOM 节点展示需要的翻译文本
 */
QString JsContent::jsCallDivTextTranslation()
{
    qDebug() << "Div text translation requested from JavaScript";
    static QJsonDocument doc;
    if (doc.isEmpty()) {
        QJsonObject object;
        object.insert("translateLabel", QApplication::translate("web", "Voice To Text"));
        object.insert("translatingLabel", QApplication::translate("web", "Converting voice to text"));
        doc.setObject(object);
        qDebug() << "Div text translation data initialized";
    }
    return doc.toJson(QJsonDocument::Compact);
}

void JsContent::jsCallScrollChange(int scrollTop)
{
    qInfo() << "Scroll change called from JavaScript, scrollTop:" << scrollTop;
    emit scrollTopChange(scrollTop == 0);
    qInfo() << "Scroll change handling finished";
}

/**
   @brief Js前端调用停止播放
 */
void JsContent::jsCallPlayVoiceStop()
{
    qInfo() << "Play voice stop called from JavaScript";
    Q_EMIT playVoiceStop();
    qInfo() << "Play voice stop handling finished";
}

/**
   @brief 变更播放进度，仅在手动拖拽进度条时触发
   @param progressMs 拖拽后的进度毫秒(ms)值
 */
void JsContent::jsCallVoiceProgressChange(qint64 progressMs)
{
    qInfo() << "Voice progress change called from JavaScript, progressMs:" << progressMs;
    Q_EMIT playVoiceProgressChange(progressMs);
    qInfo() << "Voice progress change handling finished";
}

QVariant JsContent::callJsSynchronous(QWebEnginePage *page, const QString &funtion)
{
    qDebug() << "Executing synchronous JavaScript function";
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
        qDebug() << "Synchronous JavaScript execution completed";
    } else {
        qWarning() << "Attempted to execute JavaScript on null page";
    }
    qInfo() << "Synchronous JavaScript execution finished";
    return synResult;
}

void JsContent::jsCallSetDataFinsh()
{
    qInfo() << "Set data finish called from JavaScript";
    emit setDataFinsh();
    qInfo() << "Set data finish handling finished";
}

void JsContent::jsCallPaste(bool isVoicePaste)
{
    qInfo() << "Paste called from JavaScript, isVoicePaste:" << isVoicePaste;
    emit textPaste(isVoicePaste);
    qInfo() << "Paste handling finished";
}

void JsContent::jsCallViewPicture(const QString &imagePath)
{
    qInfo() << "View picture called from JavaScript, imagePath:" << imagePath;
    emit viewPictrue(imagePath);
    qInfo() << "View picture handling finished";
}

void JsContent::jsCallCreateNote()
{
    qInfo() << "Create note called from JavaScript";
    emit createNote();
    qInfo() << "Create note handling finished";
}

void JsContent::jsCallSaveAudio()
{
    qInfo() << "Save audio called from JavaScript";
    emit saveAudio();
    qInfo() << "Save audio handling finished";
}

void JsContent::jsCallSetClipData(const QString &text, const QString &html)
{
    qDebug() << "Setting clipboard data from JavaScript";
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
        qDebug() << "Clipboard data updated successfully";
    } else {
        qWarning() << "Failed to access clipboard";
    }
    qInfo() << "Clipboard data setting finished";
}

void JsContent::onClipChange(QClipboard::Mode mode)
{
    qInfo() << "Clipboard change detected, mode:" << mode;
    if (QClipboard::Clipboard == mode && QApplication::clipboard()->mimeData() != m_clipData) {
        qDebug() << "Clipboard content changed externally";
        emit callJsClipboardDataChanged();
    }
    qInfo() << "Clipboard change handling finished";
}
