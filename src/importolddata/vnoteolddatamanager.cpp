// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnoteolddatamanager.h"
#include "olddataloadwokers.h"

#include <DLog>

#include <QThreadPool>
#include <QDebug>

VNoteOldDataManager *VNoteOldDataManager::_instance = nullptr;
VNoteDbManager *VNoteOldDataManager::m_oldDbManger = nullptr;

/**
 * @brief VNoteOldDataManager::VNoteOldDataManager
 * @param parent
 */
VNoteOldDataManager::VNoteOldDataManager(QObject *parent)
    : QObject(parent)
{
    qInfo() << "VNoteOldDataManager constructor called";
}

/**
 * @brief VNoteOldDataManager::instance
 * @return 单例对象
 */
VNoteOldDataManager *VNoteOldDataManager::instance()
{
    // qInfo() << "VNoteOldDataManager instance requested";
    if (nullptr == _instance) {
        // qInfo() << "VNoteOldDataManager instance is nullptr, creating new instance";
        _instance = new VNoteOldDataManager();
    }

    return _instance;
}

/**
 * @brief VNoteOldDataManager::releaseInstance
 */
void VNoteOldDataManager::releaseInstance()
{
    // qInfo() << "Releasing VNoteOldDataManager instance";
    if (nullptr != _instance) {
        // qInfo() << "VNoteOldDataManager instance is not nullptr, deleting old database manager";
        delete m_oldDbManger;
        m_oldDbManger = nullptr;

        delete _instance;
        _instance = nullptr;
    }
    // qInfo() << "VNoteOldDataManager instance released";
}

/**
 * @brief VNoteOldDataManager::folders
 * @return 记事本数据
 */
VNOTE_FOLDERS_MAP *VNoteOldDataManager::folders()
{
    // qInfo() << "Getting folders data";
    return m_qspNoteFoldersMap.get();
}

/**
 * @brief VNoteOldDataManager::allNotes
 * @return 记事项数据
 */
VNOTE_ALL_NOTES_MAP *VNoteOldDataManager::allNotes()
{
    // qInfo() << "Getting all notes data";
    return m_qspAllNotes.get();
}

/**
 * @brief VNoteOldDataManager::initOldDb
 */
void VNoteOldDataManager::initOldDb()
{
    qInfo() << "Initializing old database";
    //Init old database when create data manager
    //Bcs data manager depends on db.
    m_oldDbManger = new VNoteDbManager(true, this);
    qInfo() << "Old database initialization finished";
}

/**
 * @brief VNoteOldDataManager::reqDatas
 */
void VNoteOldDataManager::reqDatas()
{
    qInfo() << "Requesting old data";
    OldDataLoadTask *pOldDataLoadTask = new OldDataLoadTask();
    pOldDataLoadTask->setAutoDelete(true);

    connect(pOldDataLoadTask, &OldDataLoadTask::finishLoad, this, &VNoteOldDataManager::onFinishLoad);

    QThreadPool::globalInstance()->start(pOldDataLoadTask);
    qInfo() << "Old data request sent";
}

/**
 * @brief VNoteOldDataManager::doUpgrade
 */
void VNoteOldDataManager::doUpgrade()
{
    qInfo() << "Starting data upgrade";
    OldDataUpgradeTask *pOldDataUpgradeTask = new OldDataUpgradeTask();
    pOldDataUpgradeTask->setAutoDelete(true);

    connect(pOldDataUpgradeTask, &OldDataUpgradeTask::finishUpgrade, this, &VNoteOldDataManager::onFinishUpgrade);
    connect(pOldDataUpgradeTask, &OldDataUpgradeTask::progressValue, this, &VNoteOldDataManager::onProgress);

    QThreadPool::globalInstance()->start(pOldDataUpgradeTask);
    qInfo() << "Data upgrade task started";
}

/**
 * @brief VNoteOldDataManager::onFinishLoad
 */
void VNoteOldDataManager::onFinishLoad()
{
    qInfo() << "Old data loading finished";
    //Just notify data ready.
    emit dataReady();
    qInfo() << "Data ready signal emitted";
}

/**
 * @brief VNoteOldDataManager::onFinishUpgrade
 */
void VNoteOldDataManager::onFinishUpgrade()
{
    qInfo() << "Data upgrade finished";
    emit upgradeFinish();
    qInfo() << "Upgrade finish signal emitted";
}

/**
 * @brief VNoteOldDataManager::onProgress
 * @param value
 */
void VNoteOldDataManager::onProgress(int value)
{
    qInfo() << "Upgrade progress:" << value;
    emit progressValue(value);
    qInfo() << "Progress value signal emitted";
}
