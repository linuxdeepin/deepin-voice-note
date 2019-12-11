#include "vnotedatamanager.h"
#include "db/vnotedbmanager.h"
#include "loadfolderworker.h"
#include "loadiconsworker.h"
#include "vnoteforlder.h"

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

VNoteFolder *VNoteDataManager::addFolder(VNoteFolder *folder)
{
    VNoteFolder* retFlder = nullptr;

    if (nullptr != folder) {
        m_qspNoteFoldersMap->lock.lockForWrite();

        VNOTE_FOLDERS_DATA_MAP::iterator it = m_qspNoteFoldersMap->folders.find(folder->id);

        if (it == m_qspNoteFoldersMap->folders.end()) {
            m_qspNoteFoldersMap->folders.insert(folder->id, folder);
        } else {
            QScopedPointer<VNoteFolder> release(*it);
            m_qspNoteFoldersMap->folders.remove(folder->id);
            m_qspNoteFoldersMap->folders.insert(folder->id, folder);
        }

        m_qspNoteFoldersMap->lock.unlock();

        retFlder = folder;
    }

    return retFlder;
}

VNoteFolder *VNoteDataManager::getFolder(qint64 folderId)
{
    VNoteFolder* retFlder = nullptr;

    m_qspNoteFoldersMap->lock.lockForRead();

    VNOTE_FOLDERS_DATA_MAP::iterator it = m_qspNoteFoldersMap->folders.find(folderId);

    if (it != m_qspNoteFoldersMap->folders.end()) {
        retFlder = *it;
    }

    m_qspNoteFoldersMap->lock.unlock();

    return  retFlder;
}

VNoteFolder *VNoteDataManager::delFolder(qint64 folderId)
{
    VNoteFolder* retFlder = nullptr;

    m_qspNoteFoldersMap->lock.lockForWrite();

    VNOTE_FOLDERS_DATA_MAP::iterator it = m_qspNoteFoldersMap->folders.find(folderId);

    if (it != m_qspNoteFoldersMap->folders.end()) {
        retFlder = *it;
        m_qspNoteFoldersMap->folders.remove(it.key());
    }

    m_qspNoteFoldersMap->lock.unlock();

    return  retFlder;
}

QImage VNoteDataManager::getDefaultIcon(qint32 index)
{
    m_iconLock.lockForRead();
    if (index < 1 || index > m_defaultIcons.size()) {
        index = 0;
    }

    QImage icon = m_defaultIcons.at(index);

    m_iconLock.unlock();

    return icon;
}

void VNoteDataManager::reqNoteDefIcons()
{
    LoadIconsWorker *iconLoadWorker = new LoadIconsWorker();//
    iconLoadWorker->setAutoDelete(true);

    QThreadPool::globalInstance()->start(iconLoadWorker, QThread::TimeCriticalPriority);
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
                    << " size:" << m_qspNoteFoldersMap->folders.size();

            m_qspNoteFoldersMap->lock.lockForWrite();

            for (auto it : m_qspNoteFoldersMap->folders){
                delete reinterpret_cast<VNOTE_FOLDERS_MAP*>(it);
            }

            m_qspNoteFoldersMap->folders.clear();

            m_qspNoteFoldersMap->lock.unlock();
        }

        m_qspNoteFoldersMap.reset(foldesMap);

        qInfo() << "Loaded new foldersMap:" << m_qspNoteFoldersMap.get()
                << " size:" << m_qspNoteFoldersMap->folders.size();

        //Object is already deleted
        m_pForldesLoadthread = nullptr;

        emit onNoteFoldersLoaded();
    });

    QThreadPool::globalInstance()->start(m_pForldesLoadthread);
}

void VNoteDataManager::reqNoteItems(qint64 folder)
{
    VNOTE_FOLDERS_DATA_MAP::iterator it = m_qspNoteFoldersMap->folders.find(folder);

    if (it != m_qspNoteFoldersMap->folders.end()) {

    } else {
        //Data not loaded, start worker thread load data first
    }
}
