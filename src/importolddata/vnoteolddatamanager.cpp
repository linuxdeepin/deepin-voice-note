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
#include "vnoteolddatamanager.h"
#include "olddataloadwokers.h"

#include <DLog>

VNoteOldDataManager* VNoteOldDataManager::_instance = nullptr;
VNoteDbManager*      VNoteOldDataManager::m_oldDbManger = nullptr;

VNoteOldDataManager::VNoteOldDataManager(QObject *parent)
    : QObject(parent)
{

}

VNoteOldDataManager* VNoteOldDataManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteOldDataManager();
    }

    return _instance;
}

void VNoteOldDataManager::releaseInstance()
{
    if (nullptr != _instance) {
        delete m_oldDbManger;
        m_oldDbManger = nullptr;

        delete _instance;
        _instance = nullptr;
    }
}

VNOTE_FOLDERS_MAP *VNoteOldDataManager::folders()
{
    return m_qspNoteFoldersMap.get();
}

VNOTE_ALL_NOTES_MAP *VNoteOldDataManager::allNotes()
{
    return m_qspAllNotes.get();
}

void VNoteOldDataManager::initOldDb()
{
    //Init old database when create data manager
    //Bcs data manager depends on db.
    m_oldDbManger = new VNoteDbManager(true);
}

void VNoteOldDataManager::reqDatas()
{
    OldDataLoadTask *pOldDataLoadTask = new OldDataLoadTask();
    pOldDataLoadTask->setAutoDelete(true);

    connect(pOldDataLoadTask,&OldDataLoadTask::finishLoad
            , this, &VNoteOldDataManager::onFinishLoad);

    QThreadPool::globalInstance()->start(pOldDataLoadTask);
}

void VNoteOldDataManager::doUpgrade()
{
    OldDataUpgradeTask *pOldDataUpgradeTask = new OldDataUpgradeTask();
    pOldDataUpgradeTask->setAutoDelete(true);

    connect(pOldDataUpgradeTask,&OldDataUpgradeTask::finishUpgrade
            , this, &VNoteOldDataManager::onFinishUpgrade);
    connect(pOldDataUpgradeTask, &OldDataUpgradeTask::progressValue
            , this, &VNoteOldDataManager::onProgress);

    QThreadPool::globalInstance()->start(pOldDataUpgradeTask);
}

void VNoteOldDataManager::onFinishLoad()
{
    //Just notify data ready.
    emit dataReady();
}

void VNoteOldDataManager::onFinishUpgrade()
{
    emit upgradeFinish();
}

void VNoteOldDataManager::onProgress(int value)
{
    emit progressValue(value);
}
