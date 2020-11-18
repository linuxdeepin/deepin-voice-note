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
#ifndef JSCONTENT_H
#define JSCONTENT_H

#include <QObject>

class JsContent : public QObject
{
    Q_OBJECT
public:
    explicit JsContent(QObject *parent = nullptr);
signals:
    void initData(const QString& jsonData/*, const QString& seachKey*/);
    void insertVoiceItem(const QString &jsonData);
    void switchPlayBtn(const QString& id, int status);
public slots:
    int playButtonClick(const QString& id, int status);
    QString getVoiceSize(qint64 millisecond);
    QString getVoiceTime(const QString &time);
};

#endif // JSCONTENT_H
