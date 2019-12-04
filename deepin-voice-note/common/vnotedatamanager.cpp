#include "common/vnotedatamanager.h"
#include "db/vnotedbmanager.h"
#include "common/loadfolderworker.h"

#include <QThreadPool>

#include <DLog>

DCORE_USE_NAMESPACE

VNoteDataManager* VNoteDataManager::_instance = nullptr;

VNoteDataManager::VNoteDataManager(QObject *parent)
    : QObject(parent)
{
    m_qspNoteFoldersMap.reset(new VNOTE_FOLDERS_MAP());
}

VNoteDataManager* VNoteDataManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteDataManager();
    }

    return _instance;
}

VNOTE_FOLDERS_MAP *VNoteDataManager::getNoteFolders()
{
    return m_qspNoteFoldersMap.get();
}

void VNoteDataManager::reqNoteFolders()
{
    m_pForldesLoadthread = new LoadFolderWorker();
    m_pForldesLoadthread->setAutoDelete(true);
    connect(m_pForldesLoadthread, &LoadFolderWorker::onFoldersLoaded,
            this, [this](VNOTE_FOLDERS_MAP* foldesMap) {
        //Release old data
        if (m_qspNoteFoldersMap != nullptr) {
            qInfo() << "Release old foldersMap:" << m_qspNoteFoldersMap.get()
                    << " size:" << m_qspNoteFoldersMap->size();

            for (auto it : (*m_qspNoteFoldersMap)){
                delete reinterpret_cast<VNOTE_FOLDERS_MAP*>(it);
            }
        }

        m_qspNoteFoldersMap->clear();
        m_qspNoteFoldersMap.reset(foldesMap);

        qInfo() << "Loaded new foldersMap:" << m_qspNoteFoldersMap.get()
                << " size:" << m_qspNoteFoldersMap->size();

        //Object is already deleted
        m_pForldesLoadthread = nullptr;

        emit onNoteFoldersLoaded();
    });

    QThreadPool::globalInstance()->start(m_pForldesLoadthread);
}

void VNoteDataManager::reqNoteItems(qint64 folder)
{
    VNOTE_FOLDERS_MAP::iterator it = m_qspNoteFoldersMap->find(folder);

    if (it != m_qspNoteFoldersMap->end()) {

    } else {
        //Data not loaded, start worker thread load data first
    }
}
