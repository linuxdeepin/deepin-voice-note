/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#ifndef VNOTEOLDDATAMANAGER_H
#define VNOTEOLDDATAMANAGER_H

#include "common/datatypedef.h"
#include "db/vnotedbmanager.h"

#include <QObject>

class OldDataLoadTask;

class VNoteOldDataManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteOldDataManager(QObject *parent = nullptr);

    static VNoteOldDataManager* instance();
    static void releaseInstance();

    VNOTE_FOLDERS_MAP*   folders();
    VNOTE_ALL_NOTES_MAP* allNotes();

    //Should be called before reqDatas
    void initOldDb();
    void reqDatas();
    void doUpgrade();
signals:
    void dataReady();
    void upgradeFinish();
    void progressValue(int value);
public slots:
    void onFinishLoad();
    void onFinishUpgrade();
    void onProgress(int value);
protected:
    static VNoteOldDataManager* _instance;
    static VNoteDbManager*      m_oldDbManger;

    QScopedPointer<VNOTE_FOLDERS_MAP> m_qspNoteFoldersMap;
    QScopedPointer<VNOTE_ALL_NOTES_MAP> m_qspAllNotes;

    friend class OldDataLoadTask;
};

#endif // VNOTEOLDDATAMANAGER_H
