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
