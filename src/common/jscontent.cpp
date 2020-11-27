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
#include "globaldef.h"

#include "common/utils.h"
#include "common/vnoteitem.h"
#include "common/metadataparser.h"
#include "db/vnoteitemoper.h"

static JsContent *jsContentInstance = nullptr;

JsContent::JsContent(QObject *parent) : QObject(parent)
{

}

JsContent *JsContent::instance()
{
    if (jsContentInstance == nullptr) {
        jsContentInstance = new JsContent;
    }
    return jsContentInstance;
}

JsContent::~JsContent()
{
    if(m_currentPlay){
        delete m_currentPlay;
    }
    if(m_currentVoiceToText){
        delete  m_currentVoiceToText;
    }
}

QString JsContent::jsCallGetVoiceSize(const QString &millisecond)
{
    return Utils::formatMillisecond(millisecond.toLong(), 0);
}

QString JsContent::jsCallGetVoiceTime(const QString &time)
{
    QDateTime dataTime = QDateTime::fromString(time, VNOTE_TIME_FMT);;
    return  Utils::convertDateTime(dataTime);
}

int JsContent::jsCallPlayVoice(const QVariant &json, const bool &bIsSame)
{
    if(m_currentPlay == nullptr){
        m_currentPlay = new VNVoiceBlock();
    }
    if(!bIsSame){
        MetaDataParser dataParser;
        dataParser.jsonParse(json,m_currentPlay);
    }
    qDebug() << bIsSame;
    emit playVoice(m_currentPlay, bIsSame);
    return 1;

}

QVariant JsContent::callJsSynchronous(QWebEnginePage *page, const QString &js)
{
    m_synResult.clear();
    if (page) {
        page->runJavaScript(js, [ = ](const QVariant & result) {
            m_synResult = result;
            m_synLoop.quit();
        });
        m_synLoop.exec();
    }
    return m_synResult;
}
