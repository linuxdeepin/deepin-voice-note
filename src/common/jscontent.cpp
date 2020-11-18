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
#include "common/utils.h"
#include "common/vnoteitem.h"

#include <QDateTime>
#include <QDebug>

static JsContent *jsInstance = nullptr;

JsContent::JsContent(QObject *parent) : QObject(parent)
{

}

QString JsContent::getVoiceSize(qint64 millisecond)
{
    return Utils::formatMillisecond(millisecond, 0);
}

QString JsContent::getVoiceTime(const QString &time)
{
    QDateTime dataTime = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");;
    return  Utils::convertDateTime(dataTime);
}

int JsContent::playButtonClick(const QString& id, int status)
{
    VNoteBlock *data = getBlock(id);
    if(data && data->getType() == VNoteBlock::Voice){
        if(status == 1){
            emit voicePlay(data->ptrVoice);
        }else {
            emit voicePause(data->ptrVoice);
        }
        return 1;
    }
    qInfo() << "can not get id:" << id;
    return 0;
}

void JsContent::setNoteItem(VNoteItem *notedata)
{
    m_notedata = notedata;
}

VNoteBlock *JsContent::getBlock(const QString &id)
{
    VNoteBlock *data = nullptr;
    if(m_notedata){
        for(auto it : m_notedata->datas.datas){
            if(id == QString::number(reinterpret_cast<qint64>(it))){
                data = it;
                break;
            }
        }
    }
    return  data;
}

JsContent* JsContent::instance()
{
    if (jsInstance == nullptr) {
        jsInstance = new JsContent;
    }
    return jsInstance;
}
