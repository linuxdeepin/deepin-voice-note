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
//录音时录音记录相关信息先存入缓存表
//录音完成会处理缓存表同时数据同步到其他数据库
//录音过程中如出现异常退出，则缓存表信息未同步，则下次启动时默认是异常记录，会清理相关资源
class VNoteDataSafefyManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDataSafefyManager(QObject *parent = nullptr);

    ~VNoteDataSafefyManager();
    //单例
    static VNoteDataSafefyManager *instance();
    //启动任务线程
    void reqSafers();
    //处理数据
    void doSafer(const VDataSafer &safer);
signals:

public slots:
    //程序启动时缓存数据加载完成
    void onSafersLoaded(SafetyDatas *safers);

protected:
    //初始化任务线程
    void initTaskWoker();

protected:
    QScopedPointer<SafetyDatas> m_qsSafetyDatas;

    VNTaskWorker *m_safetyTaskWoker {nullptr};

    LoadSafeteyDataWorker *m_pSaferLoadWorker {nullptr};

    static VNoteDataSafefyManager *_instance;
};

#endif // VNOTEDATASAFEFYMANAGER_H
