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
#include <QDateTime>
#include <QDebug>

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

void JsContent::playButtonClick(const QString& id, int status)
{
    emit switchPlayBtn(id, !status);
}
