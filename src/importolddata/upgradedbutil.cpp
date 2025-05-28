// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "upgradedbutil.h"
#include "vnoteolddatamanager.h"
#include "db/vnotedbmanager.h"
#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "globaldef.h"
#include "setting.h"

#include <DLog>

DCORE_USE_NAMESPACE

const QString UpgradeDbUtil::UPGRADE_STATE = "old.UpgradeDb/importOldDbState";

/**
 * @brief UpgradeDbUtil::UpgradeDbUtil
 */
UpgradeDbUtil::UpgradeDbUtil()
{
    qDebug() << "Creating UpgradeDbUtil instance";
}

/**
 * @brief UpgradeDbUtil::saveUpgradeState
 * @param state 升级标志
 */
void UpgradeDbUtil::saveUpgradeState(int state)
{
    qDebug() << "Saving upgrade state:" << state;
    setting::instance()->setOption(UPGRADE_STATE, state);
}

/**
 * @brief UpgradeDbUtil::readUpgradeState
 * @return 升级标志
 */
int UpgradeDbUtil::readUpgradeState()
{
    int state = Invalid;

    state = setting::instance()->getOption(UPGRADE_STATE).toInt();

    return state;
}

/**
 * @brief UpgradeDbUtil::needUpdateOldDb
 * @param state
 * @return true 需要升级
 */
bool UpgradeDbUtil::needUpdateOldDb(int state)
{
    /*
     Only need update when both old exist and
     the update state is don't UpdateDone condition
     are arrived
    */
    qDebug() << "Checking if old database needs update, current state:" << state;
    bool fNeedUpdate = false;
    bool fHaveOldDb = VNoteDbManager::hasOldDataBase();

    qDebug() << "Old database exists:" << fHaveOldDb;

    if (fHaveOldDb) {
        qDebug() << "Initializing old database";
        VNoteOldDataManager::instance()->initOldDb();
    }

    if (fHaveOldDb && (state != UpdateDone)) {
        fNeedUpdate = true;
        qInfo() << "Database update required - old database exists and not in UpdateDone state";
    } else {
        qDebug() << "No database update required";
    }

    return fNeedUpdate;
}

/**
 * @brief UpgradeDbUtil::checkUpdateState
 * @param state
 */
void UpgradeDbUtil::checkUpdateState(int state)
{
    qDebug() << "Checking update state:" << state;
    if (state == Processing) {
        qWarning() << "Upgrade old database exception detected";

        QString vnoteDatabasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                    + QDir::separator()
                                    + DEEPIN_VOICE_NOTE + QString(VNoteDbManager::DBVERSION) + QString(".db");

        QFileInfo dbFileInfo(vnoteDatabasePath);

        if (dbFileInfo.exists()) {
            qDebug() << "Removing exception database file:" << vnoteDatabasePath;
            QFile dbFile(vnoteDatabasePath);

            if (!dbFile.remove()) {
                qWarning() << "Failed to remove exception database:" << dbFile.errorString();
            } else {
                qInfo() << "Successfully removed exception database";
            }
        }
    }
}

/**
 * @brief UpgradeDbUtil::backUpOldDb
 */
void UpgradeDbUtil::backUpOldDb()
{
    qDebug() << "Starting old database backup";
    QString vnoteDatabasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                + QDir::separator();

    QString oldDbName = DEEPIN_VOICE_NOTE + QString(".db");
    QString backupPath = vnoteDatabasePath + ".backup/";

    qDebug() << "Backup path:" << backupPath;
    QFileInfo backupDir(backupPath);

    if (!backupDir.exists()) {
        qDebug() << "Creating backup directory";
        QDir().mkdir(backupDir.filePath());
    }

    QFileInfo dbFileInfo(vnoteDatabasePath + oldDbName);

    if (dbFileInfo.exists()) {
        qDebug() << "Found old database file:" << dbFileInfo.filePath();
        QDir(backupDir.filePath()).remove(backupDir.filePath() + QDir::separator() + oldDbName);

        QFile oldDBFile(dbFileInfo.filePath());

        if (!oldDBFile.rename(backupDir.filePath() + QDir::separator() + oldDbName)) {
            qWarning() << "Failed to backup old database:" << oldDBFile.errorString();
        } else {
            qInfo() << "Successfully backed up old database to:" << backupDir.filePath() + QDir::separator() + oldDbName;
        }
    } else {
        qDebug() << "No old database file found to backup";
    }
}

/**
 * @brief UpgradeDbUtil::clearVoices
 */
void UpgradeDbUtil::clearVoices()
{
    qDebug() << "Starting old voices cleanup";
    QDir oldVoiceDir(QStandardPaths::standardLocations(
                         QStandardPaths::DocumentsLocation)
                         .first()
                     + QDir::separator() + "voicenote/");

    if (oldVoiceDir.exists()) {
        qDebug() << "Found old voice directory:" << oldVoiceDir.path();
        if (!oldVoiceDir.removeRecursively()) {
            qWarning() << "Failed to clear old voices directory";
        } else {
            qInfo() << "Successfully cleared old voices directory";
        }
    } else {
        qDebug() << "No old voice directory found";
    }
}

/**
 * @brief UpgradeDbUtil::doFolderUpgrade
 * @param folder
 */
void UpgradeDbUtil::doFolderUpgrade(VNoteFolder *folder)
{
    qDebug() << "Starting folder upgrade for folder:" << (folder ? folder->name : "null");
    if (nullptr != folder) {
        VNoteFolderOper folderOper;
        VNoteFolder *newVersionFolder = folderOper.addFolder(*folder);

        if (nullptr != newVersionFolder) {
            qInfo() << "Successfully created new folder:" << newVersionFolder->name << "ID:" << newVersionFolder->id;
            doFolderNoteUpgrade(newVersionFolder->id, folder->id);
        } else {
            qWarning() << "Failed to create new folder for:" << folder->name;
        }
    } else {
        qWarning() << "Cannot upgrade null folder";
    }
}

/**
 * @brief UpgradeDbUtil::doFolderNoteUpgrade
 * @param newFolderId
 * @param oldFolderId
 */
void UpgradeDbUtil::doFolderNoteUpgrade(qint64 newFolderId, qint64 oldFolderId)
{
    VNOTE_ALL_NOTES_MAP *allNotes = VNoteOldDataManager::instance()->allNotes();

    QString appAudioPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "voicenote/";

    //Create audio dir if doesn't exist.
    if (!QFileInfo(appAudioPath).exists()) {
        qDebug() << "Creating audio directory - Path: " << appAudioPath;
        QDir().mkdir(appAudioPath);
    }

    if (nullptr != allNotes) {
        auto folderNotes = allNotes->notes.find(oldFolderId);

        if (folderNotes != allNotes->notes.end()) {
            VNoteItemOper noteOper;

            for (auto note : folderNotes.value()->folderNotes) {
                //Change the old folder id to new folder id
                note->folderId = newFolderId;
                note->noteTitle = noteOper.getDefaultNoteName(newFolderId);

                if (note->haveVoice()) {
                    VNoteBlock *ptrBlock = nullptr;

                    for (auto it : note->datas.dataConstRef()) {
                        if (it->getType() == VNoteBlock::Voice) {
                            ptrBlock = it;
                            break;
                        }
                    }

                    if (nullptr != ptrBlock && !ptrBlock->ptrVoice->voicePath.isEmpty()) {
                        QFileInfo oldFileInfo(ptrBlock->ptrVoice->voicePath);
                        qDebug() << "Found voice file:" << oldFileInfo.filePath();

                        QString newVoiceName =
                            ptrBlock->ptrVoice->createTime.toString("yyyyMMddhhmmss") + QString(".mp3");

                        QString targetPath = appAudioPath + newVoiceName;
                        qDebug() << "Copying voice to:" << targetPath;

                        QFile oldFile(ptrBlock->ptrVoice->voicePath);

                        if (!oldFile.copy(targetPath)) {
                            qWarning() << "Failed to copy voice file:" << oldFile.errorString();
                        } else {
                            qInfo() << "Successfully copied voice file to:" << targetPath;
                            ptrBlock->ptrVoice->voicePath = targetPath;
                        }
                    }
                }

                noteOper.addNote(*note);
            }
        }
    }
}
