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
#ifndef VNOTEDATASAFEFYMANAGER_H
#define VNOTEDATASAFEFYMANAGER_H

#include "common/datatypedef.h"

#include <QObject>

class VNTaskWorker;
class LoadSafeteyDataWorker;

class VNoteDataSafefyManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDataSafefyManager(QObject *parent = nullptr);

    ~VNoteDataSafefyManager();

    static VNoteDataSafefyManager *instance();

    void reqSafers();

    void doSafer(const VDataSafer &safer);
signals:

public slots:
    void onSafersLoaded(SafetyDatas *safers);

protected:
    void initTaskWoker();

protected:
    QScopedPointer<SafetyDatas> m_qsSafetyDatas;

    VNTaskWorker *m_safetyTaskWoker {nullptr};

    LoadSafeteyDataWorker *m_pSaferLoadWorker {nullptr};

    static VNoteDataSafefyManager *_instance;
};

#endif // VNOTEDATASAFEFYMANAGER_H
