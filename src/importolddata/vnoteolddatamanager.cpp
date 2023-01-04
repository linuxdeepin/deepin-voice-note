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
#include "vnoteolddatamanager.h"
#include "olddataloadwokers.h"

#include <DLog>

VNoteOldDataManager *VNoteOldDataManager::_instance = nullptr;
VNoteDbManager *VNoteOldDataManager::m_oldDbManger = nullptr;

/**
 * @brief VNoteOldDataManager::VNoteOldDataManager
 * @param parent
 */
VNoteOldDataManager::VNoteOldDataManager(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief VNoteOldDataManager::instance
 * @return 单例对象
 */
VNoteOldDataManager *VNoteOldDataManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteOldDataManager();
    }

    return _instance;
}

/**
 * @brief VNoteOldDataManager::releaseInstance
 */
void VNoteOldDataManager::releaseInstance()
{
    if (nullptr != _instance) {
        delete m_oldDbManger;
        m_oldDbManger = nullptr;

        delete _instance;
        _instance = nullptr;
    }
}

/**
 * @brief VNoteOldDataManager::folders
 * @return 记事本数据
 */
VNOTE_FOLDERS_MAP *VNoteOldDataManager::folders()
{
    return m_qspNoteFoldersMap.get();
}

/**
 * @brief VNoteOldDataManager::allNotes
 * @return 记事项数据
 */
VNOTE_ALL_NOTES_MAP *VNoteOldDataManager::allNotes()
{
    return m_qspAllNotes.get();
}

/**
 * @brief VNoteOldDataManager::initOldDb
 */
void VNoteOldDataManager::initOldDb()
{
    //Init old database when create data manager
    //Bcs data manager depends on db.
    m_oldDbManger = new VNoteDbManager(true, this);
}

/**
 * @brief VNoteOldDataManager::reqDatas
 */
void VNoteOldDataManager::reqDatas()
{
    OldDataLoadTask *pOldDataLoadTask = new OldDataLoadTask();
    pOldDataLoadTask->setAutoDelete(true);

    connect(pOldDataLoadTask, &OldDataLoadTask::finishLoad, this, &VNoteOldDataManager::onFinishLoad);

    QThreadPool::globalInstance()->start(pOldDataLoadTask);
}

/**
 * @brief VNoteOldDataManager::doUpgrade
 */
void VNoteOldDataManager::doUpgrade()
{
    OldDataUpgradeTask *pOldDataUpgradeTask = new OldDataUpgradeTask();
    pOldDataUpgradeTask->setAutoDelete(true);

    connect(pOldDataUpgradeTask, &OldDataUpgradeTask::finishUpgrade, this, &VNoteOldDataManager::onFinishUpgrade);
    connect(pOldDataUpgradeTask, &OldDataUpgradeTask::progressValue, this, &VNoteOldDataManager::onProgress);

    QThreadPool::globalInstance()->start(pOldDataUpgradeTask);
}

/**
 * @brief VNoteOldDataManager::onFinishLoad
 */
void VNoteOldDataManager::onFinishLoad()
{
    //Just notify data ready.
    emit dataReady();
}

/**
 * @brief VNoteOldDataManager::onFinishUpgrade
 */
void VNoteOldDataManager::onFinishUpgrade()
{
    emit upgradeFinish();
}

/**
 * @brief VNoteOldDataManager::onProgress
 * @param value
 */
void VNoteOldDataManager::onProgress(int value)
{
    emit progressValue(value);
}
