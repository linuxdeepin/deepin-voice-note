#include "upgradedbutil.h"
#include "vnoteolddatamanager.h"
#include "db/vnotedbmanager.h"
#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "globaldef.h"

#include <DLog>

const QString UpgradeDbUtil::UPGRADE_STATE = "UpgradeDb/importOldDbState";

UpgradeDbUtil::UpgradeDbUtil()
{
}

void UpgradeDbUtil::saveUpgradeState(QSettings &setting, int state)
{
    setting.setValue(UPGRADE_STATE, state);
}

int UpgradeDbUtil::readUpgradeState(QSettings &setting)
{
    int state = Invalid;

    state = setting.value(UPGRADE_STATE, state).toInt();

    return state;
}

bool UpgradeDbUtil::needUpdateOldDb(int state)
{
    /*
     Only need update when both old exist and
     the update state is don't UpdateDone condition
     are arrived
    */
    bool fNeedUpdate = false;
    bool fHaveOldDb  = VNoteDbManager::hasOldDataBase();

    if (fHaveOldDb) {
        VNoteOldDataManager::instance()->initOldDb();
    }

    if (fHaveOldDb && (state !=UpdateDone)) {
        fNeedUpdate = true;
    }

    return fNeedUpdate;
}

void UpgradeDbUtil::checkUpdateState(int state)
{
    if (state == Processing) {
        qInfo() << "Upgrade old database exception dectected.";

        QString vnoteDatabasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + QDir::separator()
                + DEEPIN_VOICE_NOTE + QString(VNoteDbManager::DBVERSION) + QString(".db");

        QFileInfo dbFileInfo(vnoteDatabasePath);

        if (dbFileInfo.exists()) {
            QFile dbFile(vnoteDatabasePath);

            if (!dbFile.remove()) {
                qInfo() << "Remove exception db error:" << dbFile.errorString();
            }
        }
    }
}

void UpgradeDbUtil::backUpOldDb()
{
    QString vnoteDatabasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + QDir::separator();

    QString oldDbName = DEEPIN_VOICE_NOTE + QString(".db");

    QFileInfo backupDir(vnoteDatabasePath+".backup/");

    if (!backupDir.exists()) {
        QDir().mkdir(backupDir.filePath());
    }

    QFileInfo dbFileInfo(vnoteDatabasePath+oldDbName);

    if (dbFileInfo.exists()) {
        QDir(backupDir.filePath()).remove(backupDir.filePath()+QDir::separator()+oldDbName);

        QFile oldDBFile(dbFileInfo.filePath());

        if (!oldDBFile.rename(backupDir.filePath()+QDir::separator()+oldDbName)) {
            qInfo() << "Backup old database failded." << oldDBFile.errorString();
        }
    }
}

void UpgradeDbUtil::doFolderUpgrade(VNoteFolder *folder)
{
    qInfo() << "" << folder;

    if (nullptr !=folder) {
        VNoteFolderOper folderOper;
        VNoteFolder *newVersionFolder = folderOper.addFolder(*folder);

        if (nullptr != newVersionFolder) {
            doFolderNoteUpgrade(newVersionFolder->id, folder->id);
        }
    }
}

void UpgradeDbUtil::doFolderNoteUpgrade(qint64 newFolderId, qint64 oldFolderId)
{
    VNOTE_ALL_NOTES_MAP* allNotes = VNoteOldDataManager::instance()->allNotes();

    QString appAudioPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+ QDir::separator()+"/voicenote/";

    //Create audio dir if doesn't exist.
    if (!QFileInfo(appAudioPath).exists()) {
        QDir().mkdir(appAudioPath);
    }

    if (nullptr != allNotes) {

        auto folderNotes = allNotes->notes.find(oldFolderId);

        if (folderNotes != allNotes->notes.end()) {

            VNoteItemOper noteOper;

            for (auto note : folderNotes.value()->folderNotes) {
                //Change the old folder id to new folder id
                note->folderId  = newFolderId;
                note->noteTitle = noteOper.getDefaultNoteName(newFolderId);

                if (note->haveVoice()) {
                    VNoteBlock* ptrBlock = note->datas.dataConstRef().at(0);

                    if (!ptrBlock->ptrVoice->voicePath.isEmpty()) {
                        QFileInfo oldFileInfo(ptrBlock->ptrVoice->voicePath);

                        QString targetPath = appAudioPath+oldFileInfo.fileName();

                        QFile oldFile(ptrBlock->ptrVoice->voicePath);

                        if (!oldFile.copy(targetPath)) {
                            qInfo() << "Copy file failed:" << ptrBlock->ptrVoice->voicePath
                                    << " error:" << oldFile.errorString();
                        }
                    }
                }

                noteOper.addNote(*note);
            }
        }

    }
}
