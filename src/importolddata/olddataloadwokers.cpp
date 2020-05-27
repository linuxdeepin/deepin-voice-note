#include "olddataloadwokers.h"
#include "olddbvisistors.h"
#include "upgradedbutil.h"
#include "vnoteolddatamanager.h"

#include <DLog>

OldDataLoadTask::OldDataLoadTask(QObject *parent)
    :VNTask(parent)
{

}

void OldDataLoadTask::run()
{
    VNOTE_FOLDERS_MAP * foldersMap = new VNOTE_FOLDERS_MAP();
    VNoteOldDataManager::instance()->m_qspNoteFoldersMap.reset(foldersMap);

    //DataManager should set autoRelease flag
    foldersMap->autoRelease = true;

    OldFolderQryDbVisitor folderVisitor(VNoteOldDataManager::instance()->m_oldDbManger->getVNoteDb(), nullptr, foldersMap);

    if (!VNoteDbManager::instance()->queryData(&folderVisitor) ) {
      qCritical() << "Query old folder faild!";
    }


    VNOTE_ALL_NOTES_MAP * notesMap = new VNOTE_ALL_NOTES_MAP();
    VNoteOldDataManager::instance()->m_qspAllNotes.reset(notesMap);

    //DataManager data should set autoRelease flag
    notesMap->autoRelease = true;


    OldNoteQryDbVisitor noteVisitor(VNoteOldDataManager::instance()->m_oldDbManger->getVNoteDb(), nullptr, notesMap);

    if (!VNoteDbManager::instance()->queryData(&noteVisitor) ) {
        qCritical() << "Query old notes faild!";
    }

    emit finishLoad();
}

OldDataUpgradeTask::OldDataUpgradeTask(QObject *parent)
    :VNTask(parent)
{

}

void OldDataUpgradeTask::run()
{
    int folderCount = VNoteOldDataManager::instance()->folders()->folders.count();
    int progress = 0;


    int index = 1;
    for (auto it : VNoteOldDataManager::instance()->folders()->folders) {
        int curProgressValue = ((index)*100)/folderCount;

        //Do folder upgrade here.
        UpgradeDbUtil::doFolderUpgrade(it);

        if (progress != curProgressValue) {
            progress = curProgressValue;
            emit progressValue(progress);
        }

        index++;
    }

    emit finishUpgrade();
}
