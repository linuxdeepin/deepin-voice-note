/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "jscontent.h"

#include <QFile>
#include <QVariant>
#include <QEventLoop>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

#include <DApplication>

DWIDGET_USE_NAMESPACE

static JsContent *jsContentInstance = nullptr;

JsContent::JsContent(QObject *parent)
    : QObject(parent)
{
}

JsContent *JsContent::instance()
{
    if (jsContentInstance == nullptr) {
        jsContentInstance = new JsContent;
    }
    return jsContentInstance;
}

/**
 * @brief JsContent::insertImages
 * 判断图片路径是否有效，存在有效路径则创建副本传到web端中
 * @param filePaths 图片路径
 * @return 此次操作是否有效
 */
bool JsContent::insertImages(QStringList filePaths)
{
    int count = 0;
    QStringList paths;
    //获取文件夹路径
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/images";
    //创建文件夹
    QDir().mkdir(dirPath);
    //获取时间戳
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString date = currentDateTime.toString("yyyyMMddhhmmss");

    for (auto path : filePaths) {
        QFileInfo fileInfo(path);
        QString suffix = fileInfo.suffix();
        if (!(suffix == "jpg" || suffix == "png" || suffix == "bmp")) {
            continue;
        }
        //创建文件路径
        QString newPath = QString("%1/%2_%3.%4").arg(dirPath).arg(date).arg(++count).arg(suffix);
        if (QFile::copy(path, newPath)) {
            paths.push_back(newPath);
        }
    }
    if (paths.size() == 0) {
        return false;
    }
    emit callJsInsertImages(paths);
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
    //获取文件夹路径
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/images";
    //创建文件夹
    QDir().mkdir(dirPath);
    //保存文件，文件名为当前年月日时分秒
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString fileName = currentDateTime.toString("yyyyMMddhhmmss.png");
    QString imgPath = dirPath + "/" + fileName;

    //保存文件
    if (!image.save(imgPath)) {
        return false;
    }
    emit callJsInsertImages(QStringList(imgPath));
    return true;
}

void JsContent::jsCallTxtChange()
{
    emit textChange();
}

void JsContent::jsCallChannleFinish()
{
    emit loadFinsh();
}

void JsContent::jsCallPopupMenu(int type, const QVariant &json)
{
    emit popupMenu(type, json);
}

void JsContent::jsCallPlayVoice(const QVariant &json, bool bIsSame)
{
    emit playVoice(json, bIsSame);
}

QString JsContent::jsCallGetTranslation()
{
    static QJsonDocument doc;
    if (doc.isEmpty()) {
        QJsonObject object;
        object.insert("fontsize", DApplication::translate("web", "Font size"));
        object.insert("forecolor", DApplication::translate("web", "Font color"));
        object.insert("backcolor", DApplication::translate("web", "Text highlight color"));
        object.insert("bold", DApplication::translate("web", "Bold"));
        object.insert("italic", DApplication::translate("web", "Italic"));
        object.insert("underline", DApplication::translate("web", "Underline"));
        object.insert("strikethrough", DApplication::translate("web", "Strikethrough"));
        object.insert("ul", DApplication::translate("web", "Bullets"));
        object.insert("ol", DApplication::translate("web", "Numbering"));
        object.insert("more", DApplication::translate("web", "More colors"));
        object.insert("recentlyUsed", DApplication::translate("web", "Recent"));
        doc.setObject(object);
    }
    return doc.toJson(QJsonDocument::Compact);
}

QVariant JsContent::callJsSynchronous(QWebEnginePage *page, const QString &funtion)
{
    QVariant synResult;
    QEventLoop synLoop;
    if (page) {
        page->runJavaScript(funtion, [&](const QVariant &result) {
            synResult = result;
            synLoop.quit();
        });
        synLoop.exec();
    }
    return synResult;
}

void JsContent::jsCallSetDataFinsh()
{
    emit setDataFinsh();
}

void JsContent::jsCallPaste(bool isVoicePaste)
{
    emit textPaste(isVoicePaste);
}

void JsContent::jsCallViewPicture(const QString &imagePath)
{
    emit viewPictrue(imagePath);
}

void JsContent::jsCallCreateNote()
{
    emit createNote();
}
