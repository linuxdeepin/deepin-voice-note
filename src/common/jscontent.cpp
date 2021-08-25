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

#include <QApplication>
#include <QFile>
#include <QVariant>
#include <QEventLoop>

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
 * 判断图片路径是否有效，存在有效路径将其传到web端中
 * @param filePaths
 * @return 此次操作是否有效
 */
bool JsContent::insertImages(QStringList filePaths)
{
    for (int i = filePaths.size() - 1; i >= 0; --i) {
        //判断路径后缀
        if (!(filePaths.at(i).endsWith(".jpg") || filePaths.at(i).endsWith(".png") || filePaths.at(i).endsWith(".bmp"))) {
            filePaths.removeAt(i);
            continue;
        }
        //判断文件是否存在
        if (!QFile(filePaths.at(i)).exists()) {
            filePaths.removeAt(i);
        }
    }
    //无有效图片路径
    if (filePaths.size() == 0) {
        return false;
    }
    emit callJsInsertImages(filePaths);
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

void JsContent::jsCallPopupMenu(int type, int x, int y, const QVariant &json)
{
    emit popupMenu(type, x, y, json);
}

void JsContent::jsCallPlayVoice(const QVariant &json, bool bIsSame)
{
    emit playVoice(json, bIsSame);
}

QString JsContent::getTranslation(const QString &context, const QString &key)
{
    if (context.isEmpty()) {
        return QObject::tr(key.toLatin1().data());
    }
    return QApplication::translate(context.toLatin1().data(), key.toLatin1().data());
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

void JsContent::jsCallPaste()
{
    emit textPaste();
}
