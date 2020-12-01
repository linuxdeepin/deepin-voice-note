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
#include "common/actionmanager.h"
#include "common/utils.h"
#include "common/vnoteitem.h"
#include "common/metadataparser.h"
#include "db/vnoteitemoper.h"
#include "common/opsstateinterface.h"

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
    if (m_currentPlay) {
        delete m_currentPlay;
    }
    if (m_currentVoice) {
        delete  m_currentVoice;
    }
}

VNVoiceBlock *JsContent::getCurrentVoice()
{
    return  m_currentVoice;
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
    if (m_currentPlay == nullptr) {
        m_currentPlay = new VNVoiceBlock();
    }
    if (!bIsSame) {
        MetaDataParser dataParser;
        dataParser.jsonParse(json, m_currentPlay);
    }
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

void JsContent::jsCallPopVoiceMenu(const QVariant &json)
{
    if (m_currentVoice == nullptr) {
        m_currentVoice = new VNVoiceBlock();
    }
    MetaDataParser dataParser;
    dataParser.jsonParse(json, m_currentVoice);
    bool enable = !OpsStateInterface::instance()->isVoice2Text() && m_currentVoice->blockText.isEmpty();
    ActionManager::Instance()->enableAction(ActionManager::DetailVoice2Text, enable);
    ActionManager::Instance()->detialVoiceMenu()->exec(QCursor::pos());
}

void JsContent::jsCallTxtChange()
{
    emit textChange();
}
