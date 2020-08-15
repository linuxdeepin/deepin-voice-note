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
#ifndef UPGRADEDBUTIL_H
#define UPGRADEDBUTIL_H
#include "common/datatypedef.h"

#include <QtCore>

class UpgradeDbUtil
{
public:
    UpgradeDbUtil();

    static const QString UPGRADE_STATE;

    enum State {
        Invalid = 0,
        Loading,
        Processing,
        UpdateDone,
    };
    //设置升级标志
    static void saveUpgradeState(int state);
    //读取升级标志
    static int readUpgradeState();
    //是否需要升级
    static bool needUpdateOldDb(int state);
    //检查升级状态
    static void checkUpdateState(int state);
    //老数据库备份
    static void backUpOldDb();
    //删除语音文件
    static void clearVoices();
    //记事本升级
    static void doFolderUpgrade(VNoteFolder *folder);
    //记事项升级
    static void doFolderNoteUpgrade(qint64 newFolderId, qint64 oldFolderId);
};

#endif // UPGRADEDBUTIL_H
