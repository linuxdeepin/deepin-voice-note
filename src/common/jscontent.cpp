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
    m_synResult.clear();
    if (page) {
        page->runJavaScript(funtion, [=](const QVariant &result) {
            m_synResult = result;
            m_synLoop.quit();
        });
        m_synLoop.exec();
    }
    return m_synResult;
}
