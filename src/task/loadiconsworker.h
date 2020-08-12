/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
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
#ifndef LOADICONSWORKER_H
#define LOADICONSWORKER_H

#include "vntask.h"

#include <QObject>
#include <QRunnable>

class LoadIconsWorker : public VNTask
{
    Q_OBJECT
public:
    explicit LoadIconsWorker(QObject *parent = nullptr);
    QPixmap greyPix(QPixmap pix);

    const int DEFAULTICONS_COUNT = 10;

protected:
    virtual void run();

signals:

public slots:
};

#endif // LOADICONSWORKER_H
