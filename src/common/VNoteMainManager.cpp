// Copyright (C) 2019 ~ 2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatypedef.h"
#include "vnoteitemoper.h"
#include "vnotefolderoper.h"
#include "VNoteMainManager.h"
#include "jscontent.h"
#include "webrichetextmanager.h"
#include "setting.h"
#include "task/exportnoteworker.h"
#include "globaldef.h"
#include "actionmanager.h"
#include "utils.h"
#include "vtextspeechandtrmanager.h"
#include "handler/web_engine_handler.h"
#include "handler/vnote_message_dialog_handler.h"
#include "handler/voice_recoder_handler.h"
#include "handler/voice_to_text_task_manager.h"
#include "audio/recording_curves.h"
#include "dbus/VoiceNoteDBusService.h"

#include <QThreadPool>
#include <QQmlApplicationEngine>
// 条件编译：根据 Qt 版本包含不同的 WebEngine 头文件
#ifndef USE_QT5
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif
#include <QStringList>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QDebug>
#include <QImageReader>
#include <QTimer>
#include <QSet>

#include <DSysInfo>


namespace {
QString folderIdKey(qint64 folderId)
{
    return QString::number(folderId);
}

QVariantMap folderCountsForIds(const QSet<int> &folderIds)
{
    QVariantMap counts;
    VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
    if (!noteAll)
        return counts;

    noteAll->lock.lockForRead();
    for (int folderId : folderIds) {
        int count = 0;
        auto folderIt = noteAll->notes.constFind(folderId);
        if (folderIt != noteAll->notes.constEnd() && folderIt.value()) {
            VNOTE_ITEMS_MAP *folderNotes = folderIt.value();
            folderNotes->lock.lockForRead();
            count = folderNotes->folderNotes.size();
            folderNotes->lock.unlock();
        }
        counts.insert(folderIdKey(folderId), count);
    }
    noteAll->lock.unlock();
    return counts;
}
}

VNoteMainManager::VNoteMainManager()
{
    qInfo() << "VNoteMainManager constructor called";
}

// 写一个单例模式的类
VNoteMainManager *VNoteMainManager::instance()
{
    qInfo() << "VNoteMainManager instance requested";
    static VNoteMainManager instance;
    return &instance;
}

void VNoteMainManager::initNote()
{
    qInfo() << "Initializing note manager";
    m_richTextManager = new WebRichTextManager();
    initConnections();
    initData();

    int savedAudioSource = loadAudioSource();
    qInfo() << "Loading saved audio source setting:" << savedAudioSource;
    VoiceRecoderHandler::instance()->changeMode(savedAudioSource);

    m_dbusService = new VoiceNoteDBusService(this);
    m_dbusService->initDBusService();
    
    qInfo() << "Note manager initialization finished";
}

void VNoteMainManager::initData()
{
    qInfo() << "Initializing data";
    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    QThread::msleep(200);
    VNoteDataManager::instance()->reqNoteItems();
    qInfo() << "Data initialization finished";
}

void VNoteMainManager::initConnections()
{
    qInfo() << "Initializing connections";
    connect(VNoteDataManager::instance(), &VNoteDataManager::onAllDatasReady,
            this, &VNoteMainManager::onVNoteFoldersLoaded);
    connect(m_richTextManager, &WebRichTextManager::needUpdateNote, this, &VNoteMainManager::needUpdateNote);
    connect(m_richTextManager, &WebRichTextManager::noteTextChanged, this, &VNoteMainManager::onNoteChanged, Qt::QueuedConnection);
    connect(m_richTextManager, &WebRichTextManager::updateSearch, this, &VNoteMainManager::updateSearch);
    connect(m_richTextManager, &WebRichTextManager::scrollChange, this, &VNoteMainManager::scrollChange);
    connect(m_richTextManager, &WebRichTextManager::finishedUpdateNote, this, &VNoteMainManager::onRichTextSaveFinished);
    connect(VoiceRecoderHandler::instance(), &VoiceRecoderHandler::finishedRecod, this, &VNoteMainManager::insertVoice);
    qInfo() << "Connections initialized";
}

QObject *jsContent_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    qInfo() << "jsContent_provider called";
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return JsContent::instance();
}

QObject *mainManager_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    qInfo() << "mainManager_provider called";
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return VNoteMainManager::instance();
}

QObject *actionManager_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    qInfo() << "actionManager_provider called";
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return ActionManager::instance();
}

QObject *voiceRecoder_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    qInfo() << "voiceRecoder_provider called";
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return VoiceRecoderHandler::instance();
}

void VNoteMainManager::initQMLRegister()
{
    qInfo() << "Initializing QML registration";
    qmlRegisterSingletonType<VNoteMainManager>("VNote", 1, 0, "VNoteMainManager", mainManager_provider);
    qmlRegisterSingletonType<JsContent>("VNote", 1, 0, "Webobj", jsContent_provider);
    // 菜单项管理
    qmlRegisterSingletonType<ActionManager>("VNote", 1, 0, "ActionManager", actionManager_provider);

    // QML组件访问后端调用
    qmlRegisterType<WebEngineHandler>("VNote", 1, 0, "WebEngineHandler");
    qmlRegisterType<VNoteMessageDialogHandler>("VNote", 1, 0, "VNoteMessageDialogHandler");
    qmlRegisterSingletonType<VoiceRecoderHandler>("VNote", 1, 0, "VoiceRecoderHandler", voiceRecoder_provider);

    qmlRegisterType<RecordingCurves>("VNote", 1, 0, "RecordingCurves");
    qInfo() << "QML registration finished";
}

void VNoteMainManager::onVNoteFoldersLoaded()
{
    qInfo() << "VNote folders loaded";
    // #ifdef IMPORT_OLD_VERSION_DATA
    //     if (m_fNeedUpgradeOldDb) {
    //         //Start upgrade if all components are ready.
    //         m_upgradeView->startUpgrade();
    //         return;
    //     }
    // #endif

    // If have folders show note view,else show
    // default home page
    if (loadNotepads() > 0) {
        //     switchWidget(WndNoteShow);
        // } else {
        //     switchWidget(WndHomePage);
    }

    //     PerformanceMonitor::initializeAppFinish();

    //     //注册文件清理工作
    //     FileCleanupWorker *pFileCleanupWorker =
    //         new FileCleanupWorker(VNoteDataManager::instance()->getAllNotesInFolder(), this);
    //     pFileCleanupWorker->setAutoDelete(true);
    //     pFileCleanupWorker->setObjectName("FileCleanupWorker");
    //     QThreadPool::globalInstance()->start(pFileCleanupWorker);
    qInfo() << "VNote folders loaded handling finished";
}

/**
 * @brief VNoteMainManager::loadNotepads
 * @return 记事本数量
 */
int VNoteMainManager::loadNotepads()
{
    qDebug() << "Loading notepads";
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    QStringList persistedSort;
    QString value = setting::instance()->getOption(VNOTE_FOLDER_SORT).toString();
    if (!value.isEmpty()) {
        persistedSort = value.split(",", Qt::SkipEmptyParts);
        qDebug() << "Loaded folder sort order:" << persistedSort;
    }

    int folderCount = 0;
    QList<QVariantMap> foldersDataList;

    if (folders) {
        qInfo() << "folders is not nullptr";
        folders->lock.lockForRead();
        folderCount = folders->folders.size();
        qDebug() << "Found" << folderCount << "folders";

        QSet<QString> realFolderIds;
        QSet<QString> seenFolderIds;
        QStringList sanitizedSort;
        for (VNoteFolder *folder : folders->folders) {
            if (folder)
                realFolderIds.insert(folderIdKey(folder->id));
        }

        for (const QString &rawId : persistedSort) {
            bool ok = false;
            const qint64 folderId = rawId.toLongLong(&ok);
            const QString normalizedId = folderIdKey(folderId);
            if (!ok || !realFolderIds.contains(normalizedId) || seenFolderIds.contains(normalizedId)) {
                qWarning() << "Ignoring invalid folder sort entry:" << rawId;
                continue;
            }
            sanitizedSort.append(normalizedId);
            seenFolderIds.insert(normalizedId);
        }

        for (VNoteFolder *folder : folders->folders) {
            if (!folder)
                continue;
            const QString id = folderIdKey(folder->id);
            if (!seenFolderIds.contains(id)) {
                sanitizedSort.append(id);
                seenFolderIds.insert(id);
            }
        }

        m_folderSort = sanitizedSort;
        if (value != m_folderSort.join(",")) {
            setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
            qDebug() << "Sanitized folder sort order:" << m_folderSort;
        }

        for (VNoteFolder *folder : folders->folders) {
            if (!folder)
                continue;

            QVariantMap data;
            data.insert(FOLDER_NAME_KEY, folder->name);
            data.insert(FOLDER_COUNT_KEY, QString::number(folder->getNotesCount()));
            data.insert(FOLDER_ICON_KEY, QString::number(folder->defaultIcon));
            data.insert(FOLDER_ID_KEY, folder->id);
            folder->sortNumber = m_folderSort.indexOf(folderIdKey(folder->id));
            data.insert(FOLDER_SORT_KEY, folder->sortNumber);

            foldersDataList.append(data);
        }

        folders->lock.unlock();
        // 将foldersDataList按照sortNumber排序
        std::sort(foldersDataList.begin(), foldersDataList.end(),
                  [](const QVariantMap &a, const QVariantMap &b) {
                      return a[FOLDER_SORT_KEY].toInt() < b[FOLDER_SORT_KEY].toInt();
                  });
        qDebug() << "Folder data sorted";
    } else {
        qWarning() << "Cannot load notepads: folders map is null";
    }

    emit finishedFolderLoad(foldersDataList);
    qDebug() << "Folder data emitted, count:" << foldersDataList.size();
    qInfo() << "Notepads loading finished, count:" << folderCount;
    return folderCount;
}

void VNoteMainManager::vNoteFloderChanged(const int &index)
{
    qDebug() << "Changing to folder index:" << index;
    VNoteFolder *folder = getFloderByIndex(index);
    if (folder) {
        qInfo() << "folder is not nullptr";
        m_currentFolderIndex = folder->id;
        qDebug() << "Loading notes for folder ID:" << folder->id;
        if (!loadNotes(folder)) {
            qWarning() << "Failed to load notes for folder ID:" << folder->id;
        }
    } else {
        qWarning() << "Invalid folder index:" << index;
    }
    qInfo() << "Folder change finished";
}

void VNoteMainManager::vNoteFloderChangedById(const int &folderId)
{
    qDebug() << "Changing to folder ID:" << folderId;
    VNoteFolder *folder = getFloderById(folderId);
    if (folder) {
        qInfo() << "folder is not nullptr";
        m_currentFolderIndex = folder->id;
        qDebug() << "Loading notes for folder ID:" << folder->id;
        if (!loadNotes(folder)) {
            qWarning() << "Failed to load notes for folder ID:" << folder->id;
        }
    } else {
        qWarning() << "Invalid folder ID:" << folderId;
    }
    qInfo() << "Folder change by ID finished";
}

void VNoteMainManager::vNoteCreateFolder()
{
    qDebug() << "Creating new folder";
    // 录音中禁止创建记事本
    if (VoiceRecoderHandler::instance()->getRecoderType() == VoiceRecoderHandler::Recording) {
        qWarning() << "Cannot create notebook while recording";
        return;
    }
    // 播放中禁止创建记事本
    if (OpsStateInterface::instance()->isPlaying()) {
        qWarning() << "Cannot create notebook while playing";
        return;
    }
    VNoteFolder itemData;
    VNoteFolderOper folderOper;
    itemData.name = folderOper.getDefaultFolderName();

    VNoteFolder *newFolder = folderOper.addFolder(itemData);

    if (newFolder) {
        qDebug() << "New folder created with ID:" << newFolder->id;
        m_folderSort.insert(0, QString::number(newFolder->id));

        QString folderSortData = m_folderSort.join(",");
        setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);

        m_currentFolderIndex = newFolder->id;
        VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
        QVariantMap data;
        data.insert(FOLDER_NAME_KEY, newFolder->name);
        data.insert(FOLDER_COUNT_KEY, "0");
        data.insert(FOLDER_ICON_KEY, QString::number(newFolder->defaultIcon));
        data.insert(FOLDER_SORT_KEY, folders->folders.size());
        data.insert(FOLDER_ID_KEY, newFolder->id);
        addFolderFinished(data);
    } else {
        qWarning() << "Failed to create new folder";
    }
    qInfo() << "Folder creation finished";
}

bool VNoteMainManager::vNoteDeleteFolder(const int &index)
{
    qDebug() << "Deleting folder at index:" << index;
    VNoteFolder *folder = getFloderByIndex(index);
    if (!folder) {
        qWarning() << "Invalid folder index for deletion:" << index;
        return false;
    }
    return vNoteDeleteFolderById(folder->id);
}

bool VNoteMainManager::vNoteDeleteFolderById(const int &folderId)
{
    qDebug() << "Deleting folder with ID:" << folderId;
    // 录音或播放中禁止删除
    if (VoiceRecoderHandler::instance()->getRecoderType() == VoiceRecoderHandler::Recording
        || OpsStateInterface::instance()->isPlaying()) {
        qWarning() << "Cannot delete folder while recording or playing";
        return false;
    }
    VNoteFolder *folder = getFloderById(folderId);
    const bool deletingCurrentFolder = (folderId == m_currentFolderIndex);
    if (!folder) {
        qWarning() << "Folder ID for deletion is already absent, treating as stale UI cleanup:" << folderId;
        if (m_folderSort.removeAll(folderIdKey(folderId)) > 0)
            setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
        if (deletingCurrentFolder) {
            m_currentFolderIndex = -1;
            m_currentNoteId = -1;
            m_currentHasTop = 0;
            m_noteItems.clear();
            m_pendingCreateFolderIds.removeAll(folderId);
            if (m_richTextManager)
                m_richTextManager->initData(nullptr, "");
        }
        return true;
    }

    if (hasActiveVoiceToTextTaskInFolder(folder->id)) {
        qWarning() << "Cannot delete folder while voice-to-text is converting, folder ID:" << folder->id;
        return false;
    }

    const QString folderIdString = folderIdKey(folder->id);
    VNoteFolderOper folderOper(folder);
    const bool deleted = folderOper.deleteVNoteFolder(folder);
    if (!deleted) {
        qWarning() << "Failed to delete folder with ID:" << folderId;
        return false;
    }

    if (m_folderSort.removeAll(folderIdString) > 0) {
        qDebug() << "Removed folder from sort list";
    }
    m_pendingCreateFolderIds.removeAll(folderId);
    if (deletingCurrentFolder) {
        m_currentFolderIndex = -1;
        m_currentNoteId = -1;
        m_currentHasTop = 0;
        m_noteItems.clear();
        if (m_richTextManager)
            m_richTextManager->initData(nullptr, "");
    }
    setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
    qDebug() << "Folder deleted successfully";
    qInfo() << "Folder deletion finished";
    return true;
}

void VNoteMainManager::vNoteChanged(const int &index)
{
    qInfo() << "Changing to note index:" << index;
    if (index < 0) {
        qWarning() << "Invalid note index:" << index;
        return;
    }
    if (index != m_currentNoteId && !saveCurrentNoteBeforeAction(PendingAction::SwitchNote, index)) {
        return;
    }
    doSwitchNote(index);
}

void VNoteMainManager::doSwitchNote(int noteId)
{
    qDebug() << "Changing to note index:" << noteId;
    m_currentNoteId = noteId;
    VNoteItem *data = getNoteById(m_currentNoteId);
    if (!data) {
        qWarning() << "vNoteChanged resolved to null note, skipping initData";
        // 清空编辑区避免残留
        m_richTextManager->initData(nullptr, "");
        return;
    }
    m_richTextManager->initData(data, "");
    qDebug() << "Note change completed";
}

void VNoteMainManager::vNoteChangedWithUIUpdate(const int &noteId)
{
    vNoteChanged(noteId);
    
    for (int i = 0; i < m_noteItems.size(); ++i) {
        if (m_noteItems[i]->noteId == noteId) {
            emit selectNoteByIndex(i);
            break;
        }
    }
}

struct NoteCompare {
    bool operator()(const QVariantMap &a, const QVariantMap &b) const {
        // 置顶笔记排在普通笔记前面
        bool aIsTop = a.value(NOTE_ISTOP_KEY).toBool();
        bool bIsTop = b.value(NOTE_ISTOP_KEY).toBool();

        if (aIsTop != bIsTop) {
            return aIsTop > bIsTop;
        }

        // 相同置顶状态下，按最近修改时间降序排列
        QDateTime aModifyTime = QDateTime::fromString(a.value(NOTE_MODIFY_TIME_KEY).toString(), "yyyy-MM-dd hh:mm:ss");
        QDateTime bModifyTime = QDateTime::fromString(b.value(NOTE_MODIFY_TIME_KEY).toString(), "yyyy-MM-dd hh:mm:ss");

        if (aModifyTime != bModifyTime) {
            return aModifyTime > bModifyTime;
        }

        // 修改时间相同时，按笔记ID降序避免同秒创建的笔记顺序不稳定
        return a.value(NOTE_ID_KEY).toInt() > b.value(NOTE_ID_KEY).toInt();
    }
};

/**
 * @brief VNoteMainManager::loadNotes
 * @param folder
 * @return 记事项数量
 */
int VNoteMainManager::loadNotes(VNoteFolder *folder)
{
    qInfo() << "Loading notes for folder";
    m_currentHasTop = 0;
    m_noteItems.clear();
    int notesCount = -1;
    if (folder) {
        qInfo() << "folder is not nullptr";
        notesCount = 0;
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(folder->id);
        QList<QVariantMap> notesDataList;
        const int preferredNoteId = m_currentNoteId;
        if (notes) {
            qInfo() << "notes is not nullptr";
            notes->lock.lockForRead();
            for (auto it : notes->folderNotes) {
                QVariantMap data;
                data.insert(NOTE_NAME_KEY, it->noteTitle);
                data.insert(NOTE_TIME_KEY, Utils::convertDateTime(it->modifyTime));
                data.insert(NOTE_MODIFY_TIME_KEY, it->modifyTime.toString("yyyy-MM-dd hh:mm:ss"));
                data.insert(NOTE_ISTOP_KEY, it->isTop);
                data.insert(NOTE_FOLDER_ICON_KEY, QString::number(folder->defaultIcon));
                data.insert(NOTE_FOLDER_NAME_KEY, folder->name);
                data.insert(NOTE_ID_KEY, it->noteId);
                notesDataList.append(data);
                m_noteItems.append(it);
                if (it->isTop) {
                    m_currentHasTop++;
                }
                notesCount++;
            }
            notes->lock.unlock();

            std::sort(notesDataList.begin(), notesDataList.end(), NoteCompare());
        }
        int selectIndex = 0;
        bool noteFound = false;
        if (!notesDataList.isEmpty()) {
            qInfo() << "notesDataList is not empty";
            bool foundPreferredNote = false;
            if (preferredNoteId > 0) {
                for (int i = 0; i < notesDataList.size(); ++i) {
                    if (notesDataList[i].value("noteId").toInt() == preferredNoteId) {
                        selectIndex = i;
                        foundPreferredNote = true;
                        break;
                    }
                }
            }
            if (!foundPreferredNote) {
                selectIndex = 0;
                m_currentNoteId = notesDataList[selectIndex].value("noteId").toInt();
                vNoteChanged(m_currentNoteId);
                qDebug() << "Switched to note ID:" << m_currentNoteId << "at index:" << selectIndex;
            }
        } else {
            m_currentNoteId = -1;
        }
        emit updateNotes(notesDataList, selectIndex);
    }
    qInfo() << "Notes loading finished, count:" << notesCount;
    return notesCount;
}

void VNoteMainManager::insertVoice(const QString &path, qint64 size)
{
    qInfo() << "Inserting voice, path:" << path << "size:" << size;
    m_richTextManager->insertVoiceItem(path, size);
    qInfo() << "Voice insertion finished";
}

void VNoteMainManager::createNote()
{
    createNoteInFolderId(m_currentFolderIndex);
}

void VNoteMainManager::createNoteInFolderId(const int &folderId)
{
    qInfo() << "Creating new note in requested folder ID:" << folderId;
    if (!m_searchText.isEmpty()) {
        qDebug() << "Cannot create note while in search mode";
        return;
    }
    
    // 录音中禁止创建笔记
    if (VoiceRecoderHandler::instance()->getRecoderType() == VoiceRecoderHandler::Recording) {
        qWarning() << "Cannot create note while recording";
        return;
    }

    // 播放中禁止创建笔记
    if (OpsStateInterface::instance()->isPlaying()) {
        qWarning() << "Cannot create note while playing";
        return;
    }

    if (folderId == -1) {
        qWarning() << "Cannot create note: No target folder selected";
        return;
    }

    if (!getFloderById(folderId)) {
        qWarning() << "Cannot create note: target folder not found for ID:" << folderId;
        return;
    }

    if (m_richTextManager && m_richTextManager->hasPendingTextChange()) {
        const int pendingNoteId = m_richTextManager->pendingTextChangeNoteId();
        const int richTextNoteId = m_richTextManager->currentNoteId();
        if (pendingNoteId < 0 || pendingNoteId != richTextNoteId) {
            qWarning() << "Cannot create note: pending rich text change is not ready to save";
            return;
        }

        if (m_pendingAction == PendingAction::None) {
            m_pendingAction = PendingAction::CreateNote;
            m_pendingNoteId = -1;
            m_pendingCreateFolderIds.clear();
            m_pendingCreateFolderIds.append(folderId);
            m_richTextManager->requestUpdateNoteNow();
            return;
        }

        if (m_pendingAction == PendingAction::CreateNote) {
            if (!m_pendingCreateFolderIds.contains(folderId)) {
                m_pendingCreateFolderIds.append(folderId);
                qDebug() << "Queued note creation for folder ID while save is pending:" << folderId;
            } else {
                qWarning() << "Skipping duplicate pending note creation for folder ID:" << folderId;
            }
            return;
        }

        qWarning() << "Cannot create note: another pending action is in progress";
        return;
    }

    doCreateNote(folderId);
}

void VNoteMainManager::doCreateNote(int folderId)
{
    VNoteFolder *currentFolder = getFloderById(folderId);
    if (currentFolder == nullptr) {
        qWarning() << "Cannot create note: target folder not found for ID:" << folderId;
        return;
    }
    qDebug() << "Creating new note in folder ID:" << folderId;
    VNoteItem tmpNote;
    tmpNote.folderId = folderId;
    tmpNote.noteType = VNoteItem::VNT_Text;
    tmpNote.htmlCode = "<p><br></p>";
    VNoteItemOper noteOper;
    // Get default note name in the folder
    tmpNote.noteTitle = noteOper.getDefaultNoteName(tmpNote.folderId);

    VNoteItem *newNote = noteOper.addNote(tmpNote);
    if (newNote == nullptr) {
        qWarning() << "Create note failed: addNote returned null";
        return;
    }

    const bool isCurrentFolder = (folderId == m_currentFolderIndex);
    if (isCurrentFolder) {
        m_currentNoteId = newNote->noteId;
        m_noteItems.append(newNote);
    }

    QVariantMap data;
    data.insert(NOTE_NAME_KEY, newNote->noteTitle);
    data.insert(NOTE_TIME_KEY, Utils::convertDateTime(newNote->modifyTime));
    data.insert(NOTE_MODIFY_TIME_KEY, newNote->modifyTime.toString("yyyy-MM-dd hh:mm:ss"));
    data.insert(NOTE_ISTOP_KEY, newNote->isTop);
    data.insert(NOTE_FOLDER_ICON_KEY, QString::number(currentFolder->defaultIcon));
    data.insert(NOTE_FOLDER_NAME_KEY, currentFolder->name);
    data.insert(NOTE_ID_KEY, newNote->noteId);
    data.insert(FOLDER_ID_KEY, folderId);

    emit addNoteAtHead(data);
    if (isCurrentFolder && m_richTextManager)
        m_richTextManager->initData(newNote, "", true);
    qInfo() << "Note creation finished";
}

void VNoteMainManager::saveAs(const QVariantList &index, const QString &path, SaveAsType type)
{
    qDebug() << "Saving notes as" << (type == Html ? "HTML" : type == Text ? "Text" : "Voice") << "to:" << path;
    QList<VNoteItem *> noteDataList;
    ExportNoteWorker::ExportType exportType = ExportNoteWorker::ExportNothing;
    for (auto i : index) {
        if (!i.isValid())
            continue;
        VNoteItem *noteData = getNoteById(i.toInt());
        if (noteData == nullptr) {
            qWarning() << "Failed to get note by ID:" << i.toInt() << ", skipping";
            continue;
        }
        noteDataList.append(noteData);
    }
    if (noteDataList.size() == 0) {
        qWarning() << "No valid notes to export";
        return;
    }

    QString defaultName = "";
    QString urlPath = QUrl(path).path();
    QFileInfo pathInfo(urlPath);
    
    bool isDirectory = pathInfo.isDir() || urlPath.endsWith('/') || QUrl(path).fileName().isEmpty();
    qDebug() << "Path analysis - isDirectory:" << isDirectory << "pathInfo.isDir():" << pathInfo.isDir();
    
    switch (type) {
    case Html:
        qInfo() << "Exporting as HTML";
        exportType = ExportNoteWorker::ExportHtml;
        if (!isDirectory && noteDataList.size() == 1) {
            defaultName = QUrl(path).fileName();
        }
        break;
    case Text:
        qInfo() << "Exporting as Text";
        exportType = ExportNoteWorker::ExportText;
        if (!isDirectory && noteDataList.size() == 1) {
            defaultName = QUrl(path).fileName();
        }
        break;
    case Voice:
        qInfo() << "Exporting as Voice";
        exportType = ExportNoteWorker::ExportVoice;
        QDateTime now = QDateTime::currentDateTime();
        QString timeStr = now.toString("yyyyMMddhhmmss");
        defaultName = timeStr + ".mp3";
        break;
    }

    qDebug() << "Starting export with default name:" << defaultName;
    ExportNoteWorker *exportWorker = new ExportNoteWorker(
            QUrl(path).path(), exportType, noteDataList, defaultName);
    exportWorker->setAutoDelete(true);
    connect(exportWorker, &ExportNoteWorker::exportFinished, this, &VNoteMainManager::onExportFinished);
    QThreadPool::globalInstance()->start(exportWorker);
    qInfo() << "SaveAs operation started";
}

VNoteFolder *VNoteMainManager::getFloderByIndex(const int &index)
{
    qInfo() << "Getting folder by index:" << index;
    if (index < 0 || index >= m_folderSort.size()) {
        qWarning() << "Invalid folder index:" << index;
        return nullptr;
    }

    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    int tmpIndex = m_folderSort.at(index).toInt();
    if (folders) {
        qInfo() << "folders is not nullptr";
        folders->lock.lockForRead();

        VNoteFolder *folder = folders->folders.value(tmpIndex);
        folders->lock.unlock();
        return folder;
    }
    return nullptr;
}

VNoteFolder *VNoteMainManager::getFloderById(const int &id)
{
    qInfo() << "Getting folder by ID:" << id;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        qInfo() << "folders is not nullptr";
        folders->lock.lockForRead();

        QMap<qint64, VNoteFolder *>::iterator itor;
        for(itor = folders->folders.begin(); itor != folders->folders.end(); ++itor) {
            VNoteFolder *folder = itor.value();
            if (folder && folder->id == id) {
                folders->lock.unlock();
                return folder;
            }
        }

        folders->lock.unlock();
    }
    return nullptr;
}

int VNoteMainManager::getFloderIndexById(const int &id)
{
    qInfo() << "Getting folder index by ID:" << id;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (!folders) {
        qWarning() << "Cannot get folder index, folders map is null";
        return -1;
    }

    folders->lock.lockForRead();
    for (int i = 0; i < m_folderSort.size(); i++) {
        int tmpIndex = m_folderSort.at(i).toInt();
        VNoteFolder *folder = folders->folders.value(tmpIndex);
        if (folder && folder->id == id) {
            folders->lock.unlock();
            return i;
        }
    }
    folders->lock.unlock();
    return -1;
}

VNoteItem *VNoteMainManager::getNoteById(const int &id)
{
    qInfo() << "Getting note by ID:" << id;
    foreach (auto item, m_noteItems) {
        if (item->noteId == id)
            return item;
    }
    return nullptr;
}

VNoteItem *VNoteMainManager::deleteNoteById(const int &id)
{
    qInfo() << "Deleting note by ID:" << id;
    foreach (auto item, m_noteItems) {
        if (item->noteId == id) {
            m_noteItems.removeOne(item);
            return item;
        }
    }
    return nullptr;
}

bool VNoteMainManager::deleteNote(const QList<int> &index)
{
    // 删除之前清空JS详情页内容
    qDebug() << "Deleting" << index.size() << "notes";
    // 录音或播放中禁止删除
    if (VoiceRecoderHandler::instance()->getRecoderType() == VoiceRecoderHandler::Recording
        || OpsStateInterface::instance()->isPlaying()) {
        qWarning() << "Cannot delete note while recording or playing";
        return false;
    }
    for (int noteId : index) {
        if (hasActiveVoiceToTextTaskForNote(noteId)) {
            qWarning() << "Cannot delete note while voice-to-text is converting, note ID:" << noteId;
            return false;
        }
    }

    m_richTextManager->clearJSContent();
    QList<VNoteItem *> noteDataList;
    for (int i = 0; i < index.size(); i++) {
        VNoteItem *note = getNoteById(index.at(i));
        if (!note) {
            qWarning() << "Failed to get note by ID for deletion:" << index.at(i);
            continue;
        }
        noteDataList.append(note);
        if (note->isTop)
            m_currentHasTop--;
        m_noteItems.removeOne(note);
    }

    if (noteDataList.size()) {
        qWarning() << "Processing deletion of" << noteDataList.size() << "notes";
        // track deleted count per folder id for UI sync (e.g. search mode)
        QMap<int, int> folderIdToDeletedCount;
        for (auto noteData : noteDataList) {
            // 在删除前先保存folderId，避免删除后访问已释放内存
            int folderId = noteData->folderId;
            qWarning() << "Deleting note from folder ID:" << folderId;
            VNoteItemOper noteOper(noteData);
            noteOper.deleteNote();
            folderIdToDeletedCount[folderId] += 1;
        }
        // Convert QMap to QVariantMap for QML compatibility
        QVariantMap variantMap;
        for (auto it = folderIdToDeletedCount.begin(); it != folderIdToDeletedCount.end(); ++it) {
            variantMap[QString::number(it.key())] = it.value();
            qWarning() << "Folder ID" << it.key() << "deleted count:" << it.value();
        }
        qWarning() << "Emitting notesDeleted signal with" << variantMap.size() << "folders";
        emit notesDeleted(variantMap);
    } else {
        qWarning() << "No notes to delete";
        return false;
    }
    qInfo() << "Note deletion finished";
    return true;
}

bool VNoteMainManager::hasActiveVoiceToTextTaskForNote(int noteId) const
{
    const QList<VoiceToTextTask> tasks = VoiceToTextTaskManager::instance()->getTasksForNote(noteId);
    for (const auto &task : tasks) {
        if (task.status == VoiceToTextTask::Converting) {
            return true;
        }
    }
    return false;
}

bool VNoteMainManager::hasActiveVoiceToTextTaskInFolder(qint64 folderId) const
{
    VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
    if (!noteAll) {
        return false;
    }

    noteAll->lock.lockForRead();
    auto folderIt = noteAll->notes.constFind(folderId);
    if (folderIt == noteAll->notes.constEnd() || !folderIt.value()) {
        noteAll->lock.unlock();
        return false;
    }

    VNOTE_ITEMS_MAP *folderNotes = folderIt.value();
    folderNotes->lock.lockForRead();
    bool hasActiveTask = false;
    for (auto note : folderNotes->folderNotes) {
        if (note && hasActiveVoiceToTextTaskForNote(note->noteId)) {
            hasActiveTask = true;
            break;
        }
    }
    folderNotes->lock.unlock();
    noteAll->lock.unlock();

    return hasActiveTask;
}

void VNoteMainManager::moveNotes(const QVariantList &index, const int &folderIndex)
{
    qDebug() << "Moving" << index.size() << "notes to folder index:" << folderIndex;
    VNoteFolder *folder = getFloderByIndex(folderIndex);
    if (!folder) {
        qWarning() << "Invalid destination folder index for move:" << folderIndex;
        return;
    }
    moveNotesToFolderId(index, folder->id);
}

void VNoteMainManager::moveNotesToFolderId(const QVariantList &noteIds, const int &folderId)
{
    qDebug() << "Moving" << noteIds.size() << "notes to folder ID:" << folderId;
    if (noteIds.isEmpty()) {
        qWarning() << "Invalid move parameters: empty note list";
        return;
    }

    for (const QVariant &noteId : noteIds) {
        if (hasActiveVoiceToTextTaskForNote(noteId.toInt())) {
            qWarning() << "Cannot move note while voice-to-text is converting, note ID:" << noteId.toInt();
            return;
        }
    }

    VNoteFolder *folder = getFloderById(folderId);
    if (!folder) {
        qWarning() << "Invalid destination folder ID for move:" << folderId;
        return;
    }

    VNoteItemOper noteOper;
    VNOTE_ITEMS_MAP *destNotes = noteOper.getFolderNotes(folder->id);
    if (!destNotes) {
        qWarning() << "Invalid move operation, destination notes map is null";
        return;
    }

    int firstSrcFolderId = -1;
    int movedCount = 0;
    QSet<int> affectedFolderIds;
    affectedFolderIds.insert(folder->id);

    for (const QVariant &noteId : noteIds) {
        VNoteItem *note = getNoteById(noteId.toInt());
        if (!note) {
            qWarning() << "Skipping invalid note during move, note ID:" << noteId.toInt();
            continue;
        }

        const int srcFolderId = note->folderId;

        if (srcFolderId == folder->id) {
            qWarning() << "Skipping move to same folder, note ID:" << note->noteId << "folder ID:" << folder->id;
            continue;
        }

        VNOTE_ITEMS_MAP *srcNotes = noteOper.getFolderNotes(srcFolderId);
        if (!srcNotes) {
            qWarning() << "Skipping note with null source notes map, note ID:" << note->noteId << "source folder ID:" << srcFolderId;
            continue;
        }

        if (firstSrcFolderId == -1)
            firstSrcFolderId = srcFolderId;

        m_noteItems.removeOne(note);
        if (note->isTop)
            m_currentHasTop--;

        srcNotes->lock.lockForWrite();
        srcNotes->folderNotes.remove(note->noteId);
        srcNotes->lock.unlock();

        destNotes->lock.lockForWrite();
        note->folderId = folder->id;
        destNotes->folderNotes.insert(note->noteId, note);
        destNotes->lock.unlock();

        noteOper.updateFolderId(note);
        affectedFolderIds.insert(srcFolderId);
        affectedFolderIds.insert(folder->id);
        ++movedCount;
    }

    if (movedCount <= 0) {
        qWarning() << "No notes moved";
        return;
    }

    folder->maxNoteIdRef() += movedCount;
    const QVariantMap folderIdToCount = folderCountsForIds(affectedFolderIds);
    qDebug() << "Notes moved successfully";
    emit moveFinishedByFolderId(noteIds, firstSrcFolderId, folder->id, folderIdToCount);
    qInfo() << "Notes move finished";
}

void VNoteMainManager::updateTop(const int &id, const bool &top)
{
    qInfo() << "Updating top status for note ID:" << id << "to:" << top;
    VNoteItem *note = getNoteById(id);
    if (note == nullptr || note->isTop == top) {
        qWarning() << "Invalid top update operation";
        return;
    }
    VNoteItemOper noteOper(note);
    noteOper.updateTop(top);
    m_currentHasTop = top ? m_currentHasTop + 1 : m_currentHasTop - 1;

    QList<QVariantMap> notesDataList;
    for (auto it : m_noteItems) {
        QVariantMap data;
        data.insert(NOTE_NAME_KEY, it->noteTitle);
        data.insert(NOTE_TIME_KEY, Utils::convertDateTime(it->modifyTime));
        data.insert(NOTE_MODIFY_TIME_KEY, it->modifyTime.toString("yyyy-MM-dd hh:mm:ss"));
        data.insert(NOTE_ISTOP_KEY, it->isTop);
        VNoteFolder *folder = getFloderById(it->folderId);
        data.insert(NOTE_FOLDER_ICON_KEY, QString::number(folder->defaultIcon));
        data.insert(NOTE_FOLDER_NAME_KEY, folder->name);
        data.insert(NOTE_ID_KEY, it->noteId);
        notesDataList.append(data);
    }

    std::sort(notesDataList.begin(), notesDataList.end(), NoteCompare());

    auto it = std::find_if(notesDataList.begin(), notesDataList.end(), [id](const QVariantMap &item)->bool {
        return item.value("noteId").toInt() == id;
    });
    emit updateNotes(notesDataList, std::distance(notesDataList.begin(), it));
    qInfo() << "Top status update finished";
}

void VNoteMainManager::onExportFinished(int err)
{
    qInfo() << "Export finished with error code:" << err;
    Q_UNUSED(err)
    // TODO:提示保存成功
    qInfo() << "Export finished handling completed";
}

void VNoteMainManager::onNoteChanged()
{
    qInfo() << "Note changed, updating modification time";
    int changedNoteId = m_richTextManager ? m_richTextManager->pendingTextChangeNoteId() : -1;
    if (changedNoteId < 0) {
        changedNoteId = m_currentNoteId;
    }
    VNoteItem *note = getNoteById(changedNoteId);
    if (!note) {
        qWarning() << "onNoteChanged: changed note not found, id=" << changedNoteId << ", current=" << m_currentNoteId << ", skip";
        return;
    }
    note->modifyTime = QDateTime::currentDateTime();
    emit updateEditNote(changedNoteId, Utils::convertDateTime(note->modifyTime));
    qInfo() << "Note change handling finished";
}

void VNoteMainManager::updateSearch()
{
    qInfo() << "Updating search";
    if (m_searchText.isEmpty()) {
        qDebug() << "Search text is empty, skipping update";
        return;
    }
    qDebug() << "Updating search with text:" << m_searchText;
    emit updateRichTextSearch(m_searchText);
    qInfo() << "Search update finished";
}

void VNoteMainManager::onRichTextSaveFinished()
{
    if (m_pendingAction == PendingAction::None) {
        return;
    }

    if (m_richTextManager && m_richTextManager->hasPendingTextChange()) {
        qWarning() << "Pending note action canceled because rich text save did not finish successfully";
        m_pendingAction = PendingAction::None;
        m_pendingNoteId = -1;
        m_pendingCreateFolderIds.clear();
        return;
    }

    const PendingAction action = m_pendingAction;
    const int noteId = m_pendingNoteId;
    const QList<int> createFolderIds = m_pendingCreateFolderIds;
    m_pendingAction = PendingAction::None;
    m_pendingNoteId = -1;
    m_pendingCreateFolderIds.clear();

    if (action == PendingAction::SwitchNote) {
        doSwitchNote(noteId);
    } else if (action == PendingAction::CreateNote) {
        for (int folderId : createFolderIds)
            doCreateNote(folderId);
    }
}

bool VNoteMainManager::saveCurrentNoteBeforeAction(PendingAction action, int noteId)
{
    if (!m_richTextManager || !m_richTextManager->hasPendingTextChange()) {
        return true;
    }

    const int pendingNoteId = m_richTextManager->pendingTextChangeNoteId();
    const int richTextNoteId = m_richTextManager->currentNoteId();
    if (pendingNoteId < 0 || pendingNoteId != richTextNoteId) {
        return false;
    }

    if (m_pendingAction != PendingAction::None) {
        return false;
    }

    m_pendingAction = action;
    m_pendingNoteId = noteId;
    m_richTextManager->requestUpdateNoteNow();
    return false;
}

bool VNoteMainManager::getTop()
{
    qInfo() << "Getting top status:" << m_currentHasTop;
    return m_currentHasTop;
}

void VNoteMainManager::updateSort(const int &src, const int &dst)
{
    qInfo() << "Updating sort, src:" << src << "dst:" << dst;
    if (src < 0 || src >= m_folderSort.size() || dst < 0 || dst >= m_folderSort.size()) {
        qWarning() << "Invalid folder sort indexes, src:" << src << "dst:" << dst << "size:" << m_folderSort.size();
        return;
    }
    QString tmp = m_folderSort.at(src);
    m_folderSort.removeAt(src);
    m_folderSort.insert(dst, tmp);
    setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
    qInfo() << "Sort update finished";
}

void VNoteMainManager::updateSortByFolderIds(const QVariantList &folderIds)
{
    qInfo() << "Updating sort by folder IDs, count:" << folderIds.size();
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (!folders) {
        qWarning() << "Cannot update folder sort, folders map is null";
        return;
    }

    QSet<QString> realFolderIds;
    folders->lock.lockForRead();
    for (VNoteFolder *folder : folders->folders) {
        if (folder)
            realFolderIds.insert(folderIdKey(folder->id));
    }
    folders->lock.unlock();

    QSet<QString> seenFolderIds;
    QStringList normalizedSort;
    for (const QVariant &folderIdValue : folderIds) {
        bool ok = false;
        const qint64 folderId = folderIdValue.toLongLong(&ok);
        const QString id = folderIdKey(folderId);
        if (!ok || !realFolderIds.contains(id) || seenFolderIds.contains(id)) {
            qWarning() << "Ignoring invalid folder ID in sort update:" << folderIdValue;
            continue;
        }
        normalizedSort.append(id);
        seenFolderIds.insert(id);
    }

    for (const QString &id : m_folderSort) {
        if (realFolderIds.contains(id) && !seenFolderIds.contains(id)) {
            normalizedSort.append(id);
            seenFolderIds.insert(id);
        }
    }

    folders->lock.lockForRead();
    for (VNoteFolder *folder : folders->folders) {
        if (!folder)
            continue;
        const QString id = folderIdKey(folder->id);
        if (!seenFolderIds.contains(id)) {
            normalizedSort.append(id);
            seenFolderIds.insert(id);
        }
    }
    folders->lock.unlock();

    m_folderSort = normalizedSort;
    setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
    qInfo() << "Sort by folder IDs update finished";
}

void VNoteMainManager::renameFolder(const int &index, const QString &name)
{
    qDebug() << "Renaming folder at index" << index << "to:" << name;
    VNoteFolder *folder = getFloderByIndex(index);
    if (!folder) {
        qWarning() << "Invalid folder rename operation";
        qInfo() << "Folder rename finished";
        return;
    }
    renameFolderById(folder->id, name);
}

void VNoteMainManager::renameFolderById(const int &folderId, const QString &name)
{
    qDebug() << "Renaming folder with ID" << folderId << "to:" << name;
    VNoteFolder *folder = getFloderById(folderId);
    if (folder && name != folder->name) {
        VNoteFolderOper folderOper(folder);
        folderOper.renameVNoteFolder(name);
        qDebug() << "Folder renamed successfully";
    } else {
        qWarning() << "Invalid folder rename operation";
    }
    qInfo() << "Folder rename by ID finished";
}

void VNoteMainManager::renameNote(const int &index, const QString &newName)
{
    qDebug() << "Renaming note at index" << index << "to:" << newName;
    VNoteItem *item = getNoteById(index);
    if (item && !newName.isEmpty() && newName != item->noteTitle) {
        qInfo() << "rename note!";
        VNoteItemOper noteOps(item);
        if (noteOps.modifyNoteTitle(newName)) {
            qDebug() << "Note renamed successfully";
            // 播放中不重新加载列表，避免中断播放状态
            if (!OpsStateInterface::instance()->isPlaying() && !isInSearchMode()) {
                qDebug() << "Not playing, reloading current folder";
                VNoteFolder *currentFolder = getFloderById(m_currentFolderIndex);
                if (currentFolder) {
                    loadNotes(currentFolder);
                }
            } else {
                qDebug() << "Playing audio, skip reloading to preserve play state";
                // 更新标题
                item->noteTitle = newName;
                emit noteTitleChanged(index, newName);
            }
        } else {
            qWarning() << "Failed to rename note";
        }
    } else {
        qWarning() << "Invalid note rename operation";
    }
    onNoteChanged();
    qInfo() << "Note rename finished";
}

QString VNoteMainManager::getNotePlainTitle(const int &noteId)
{
    qDebug() << "Getting plain title for note ID:" << noteId;
    VNoteItem *item = getNoteById(noteId);
    if (item) {
        QString plainTitle = Utils::stripHtmlTags(item->noteTitle);
        qDebug() << "Plain title for note ID" << noteId << ":" << plainTitle;
        return plainTitle;
    } else {
        qWarning() << "Note not found for ID:" << noteId;
        return QString();
    }
}

void VNoteMainManager::vNoteSearch(const QString &text)
{
    qInfo() << "Starting search with text:" << text;
    if (!text.isEmpty()) {
        // 当当前的输入文本和前一次相同的时候，回车也需要触发操作，否则笔记有变更无法及时更新搜索结果
        qDebug() << "Starting search with text:" << text;
        m_searchText = text;
        loadSearchNotes(text);
        updateSearch();
    } else {
        qDebug() << "Search text is empty";
    }
    qInfo() << "Search operation finished";
}

void VNoteMainManager::updateNoteWithResult(const QString &result)
{
    qInfo() << "Updating note with result";
    updateNoteWithResultForNote(m_currentNoteId, result);
    qInfo() << "Note update with result finished";
}

void VNoteMainManager::updateNoteWithResultForNote(int noteId, const QString &result)
{
    VNoteItem *note = getNoteById(noteId);
    m_richTextManager->onUpdateNoteWithResult(note, result);
}

int VNoteMainManager::loadSearchNotes(const QString &key)
{
    qInfo() << "Loading search notes for key:" << key;
    if (key.isEmpty()) {
        qWarning() << "Empty search key";
        return -1;
    }
    qDebug() << "Loading search notes for key:" << key;
    VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
    QList<QVariantMap> notesDataList;
    m_noteItems.clear();
    if (noteAll) {
        qInfo() << "noteAll is not nullptr";
        noteAll->lock.lockForRead();
        for (auto &foldeNotes : noteAll->notes) {
            for (auto note : foldeNotes->folderNotes) {
                if (note->search(key)) {
                    QVariantMap data;
                    data.insert(NOTE_NAME_KEY, Utils::createRichText(note->noteTitle, key));
                    data.insert(NOTE_TIME_KEY, Utils::convertDateTime(note->modifyTime));
                    data.insert(NOTE_ISTOP_KEY, QString::number(note->isTop));
                    data.insert(NOTE_ID_KEY, note->noteId);
                    VNoteFolder *folder = getFloderById(note->folderId);
                    data.insert(NOTE_FOLDER_NAME_KEY, folder->name);
                    data.insert(NOTE_FOLDER_ICON_KEY, QString::number(folder->defaultIcon));
                    notesDataList.append(data);
                    m_noteItems.append(note);
                }
            }
        }
        noteAll->lock.unlock();
        if (notesDataList.size() == 0) {
            qDebug() << "No search results found";
            emit noSearchResult();
        } else {
            qInfo() << "notesDataList is not empty";
            //TODO:有搜索结果
            emit searchFinished(notesDataList, key);
        }
    }
    qInfo() << "Search notes loading finished, count:" << notesDataList.size();
    return notesDataList.size();
}

int VNoteMainManager::loadAudioSource()
{
    qInfo() << "Loading audio source";
    return setting::instance()->getOption(VNOTE_AUDIO_SELECT).toInt();
}

void VNoteMainManager::changeAudioSource(const int &source)
{
    qInfo() << "Changing audio source to:" << source;
    setting::instance()->setOption(VNOTE_AUDIO_SELECT, QVariant(source));
    VoiceRecoderHandler::instance()->changeMode(source);
}

bool VNoteMainManager::canInsertImages(const QList<QUrl> &filePaths) const
{
    if (filePaths.isEmpty())
        return false;

    for (const QUrl &path : filePaths) {
        if (!path.isLocalFile())
            return false;

        const QString localPath = QDir::cleanPath(path.toLocalFile());
        const QFileInfo fileInfo(localPath);
        if (!fileInfo.exists() || !fileInfo.isFile())
            return false;

        QImageReader imageReader(localPath);
        if (!imageReader.canRead())
            return false;
    }

    return true;
}

void VNoteMainManager::insertImages(const QList<QUrl> &filePaths)
{
    qDebug() << "Inserting" << filePaths.size() << "images";
    int count = 0;
    QStringList paths;
    //获取文件夹路径
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/images";
    //创建文件夹
    QDir().mkdir(dirPath);
    //获取时间戳
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString date = currentDateTime.toString("yyyyMMddhhmmss");

    for (auto path : filePaths) {
        if (!path.isLocalFile()) {
            qWarning() << "Unsupported non-local image URL:" << path;
            continue;
        }
        QString localPath = QDir::cleanPath(path.toLocalFile());
        QFileInfo fileInfo(localPath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            qWarning() << "Image file does not exist or is not a file:" << localPath;
            continue;
        }
        QImageReader imageReader(localPath);
        if (!imageReader.canRead()) {
            qWarning() << "Invalid image file:" << localPath;
            continue;
        }
        QString suffix = fileInfo.suffix().toLower();
        if (suffix.isEmpty()) {
            suffix = QString::fromLatin1(imageReader.format()).toLower();
        }
        if (suffix == QStringLiteral("jpeg")) {
            suffix = QStringLiteral("jpg");
        }
        if (suffix.isEmpty()) {
            qWarning() << "Unsupported image format:" << localPath;
            continue;
        }
        //创建文件路径
        QString newPath = QString("%1/%2_%3.%4").arg(dirPath).arg(date).arg(++count).arg(suffix);
        if (QFile::copy(localPath, newPath)) {
            const QString fileName = QString("%1_%2.%3").arg(date).arg(count).arg(suffix);
            paths.push_back(QString("images/") + fileName);
        }
    }
    if (!paths.isEmpty()) {
        qDebug() << "Inserting" << paths.size() << "images into note";
        JsContent::instance()->callJsInsertImages(paths);
    } else {
        qWarning() << "No valid images to insert";
    }
    qInfo() << "Image insertion finished";
}

void VNoteMainManager::checkNoteVoice(const QVariantList &index)
{
    qInfo() << "Checking note voice for" << index.size() << "notes";
    bool hasVoice = false;
    foreach (auto id, index) {
        int noteIndex = id.toInt();
        VNoteItem *item = getNoteById(noteIndex);
        if (item && item->haveVoice()) {
            hasVoice = true;
            break;
        }
    }
    ActionManager::instance()->enableAction(ActionManager::NoteSaveVoice, hasVoice);
    emit saveVoiceStateChanged(hasVoice);
    qInfo() << "Note voice check finished, hasVoice:" << hasVoice;
}

void VNoteMainManager::checkNoteText(const QVariantList &index)
{
    qInfo() << "Checking note text for" << index.size() << "notes";
    foreach (auto id, index) {
        int noteIndex = id.toInt();
        VNoteItem *item = getNoteById(noteIndex);
        if (item && item->haveText()) {
            ActionManager::instance()->enableAction(ActionManager::NoteSave, true);
            ActionManager::instance()->enableAction(ActionManager::SaveNoteAsText, true);
            ActionManager::instance()->enableAction(ActionManager::SaveNoteAsHtml, true);
            return;
        }
    }
    ActionManager::instance()->enableAction(ActionManager::NoteSave, false);
    ActionManager::instance()->enableAction(ActionManager::SaveNoteAsText, false);
    ActionManager::instance()->enableAction(ActionManager::SaveNoteAsHtml, false);
    qInfo() << "Note text check finished";
}

void VNoteMainManager::clearSearch()
{
    qInfo() << "Clearing search";
    m_searchText = "";
    // 发出信号清除搜索高亮
    emit updateRichTextSearch("");
    qInfo() << "Search cleared";
}

bool VNoteMainManager::isInSearchMode() const
{
    qInfo() << "Checking search mode:" << !m_searchText.isEmpty();
    return !m_searchText.isEmpty();
}

bool VNoteMainManager::hasNoteText(int noteId)
{
    VNoteItem *item = getNoteById(noteId);
    if (item) {
        return item->haveText();
    }
    return false;
}

void VNoteMainManager::preViewShortcut(const QPointF &point)
{
    // qInfo() << "Previewing shortcut at point:" << point.x() << "," << point.y();
    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    //******************************Notebooks**************************************************
    QMap<QString, QString> shortcutNotebookKeymap = {
                                                     //Notebook
                                                     {DApplication::translate("Shortcuts", "New notebook"), "Ctrl+N"},
                                                     {DApplication::translate("Shortcuts", "Rename notebook"), "F2"},
                                                     {DApplication::translate("Shortcuts", "Delete notebook"), "Delete"},
                                                     };

    QJsonObject notebookJsonGroup;
    notebookJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Notebooks"));
    QJsonArray notebookJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutNotebookKeymap.begin();
         it != shortcutNotebookKeymap.end(); ++it) {
        QJsonObject jsonItem;
        jsonItem.insert("name", it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        notebookJsonItems.append(jsonItem);
    }

    notebookJsonGroup.insert("groupItems", notebookJsonItems);
    jsonGroups.append(notebookJsonGroup);

    //******************************Notes**************************************************

    QMap<QString, QString> shortcutNoteKeymap = {
                                                 //Note
                                                 {DApplication::translate("Shortcuts", "New note"), "Ctrl+B"},
                                                 {DApplication::translate("Shortcuts", "Rename note"), "F3"},
                                                 {DApplication::translate("Shortcuts", "Delete note"), "Delete"},
                                                 {DApplication::translate("Shortcuts", "Play/Pause"), "Space"},
                                                 {DApplication::translate("Shortcuts", "Record voice"), "Ctrl+R"},
                                                 {DApplication::translate("Shortcuts", "Save note"), "Ctrl+S"},
                                                 {DApplication::translate("Shortcuts", "Save recordings"), "Ctrl+D"},
                                                 };

    QJsonObject noteJsonGroup;
    noteJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Notes"));
    QJsonArray noteJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutNoteKeymap.begin();
         it != shortcutNoteKeymap.end(); ++it) {
        QJsonObject jsonItem;
        jsonItem.insert("name", it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        noteJsonItems.append(jsonItem);
    }

    noteJsonGroup.insert("groupItems", noteJsonItems);
    jsonGroups.append(noteJsonGroup);
    //******************************Edit***************************************************
    QList<QPair<QString, QString>> shortcutEditKeymap = {
                                                         //Edit
                                                         {DApplication::translate("Shortcuts", "Select all"), "Ctrl+A"},
                                                         {DApplication::translate("Shortcuts", "Copy"), "Ctrl+C"},
                                                         {DApplication::translate("Shortcuts", "Cut"), "Ctrl+X"},
                                                         {DApplication::translate("Shortcuts", "Paste"), "Ctrl+V"},
                                                         {DApplication::translate("Shortcuts", "Undo"), "Ctrl+Z"},
                                                         {DApplication::translate("Shortcuts", "Redo"), "Ctrl+Shift+Z"},
                                                         {DApplication::translate("Shortcuts", "Delete"), "Delete"},
                                                         };

    QJsonObject editJsonGroup;
    editJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Edit"));
    QJsonArray editJsonItems;

    for (int i = 0; i < shortcutEditKeymap.count(); i++) {
        QJsonObject jsonItem;
        jsonItem.insert("name", shortcutEditKeymap[i].first);
        QString value = shortcutEditKeymap[i].second;
        jsonItem.insert("value", value.replace("Meta", "Super"));
        editJsonItems.append(jsonItem);
    }

    editJsonGroup.insert("groupItems", editJsonItems);
    jsonGroups.append(editJsonGroup);
    //******************************Setting************************************************
    QMap<QString, QString> shortcutSettingKeymap = {
                                                    //Setting
                                                    //        {DApplication::translate("Shortcuts","Close window"),         "Alt+F4"},
                                                    //        {DApplication::translate("Shortcuts","Resize window"),        "Ctrl+Alt+F"},
                                                    //        {DApplication::translate("Shortcuts","Find"),                 "Ctrl+F"},
                                                    {DApplication::translate("Shortcuts", "Help"), "F1"},
                                                    {DApplication::translate("Shortcuts", "Display shortcuts"), "Ctrl+Shift+?"},
                                                    };

    QJsonObject settingJsonGroup;
    settingJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Settings"));
    QJsonArray settingJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutSettingKeymap.begin();
         it != shortcutSettingKeymap.end(); ++it) {
        QJsonObject jsonItem;
        jsonItem.insert("name", it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        settingJsonItems.append(jsonItem);
    }

    settingJsonGroup.insert("groupItems", settingJsonItems);
    jsonGroups.append(settingJsonGroup);

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    QPoint pos(static_cast<int>(point.x()), static_cast<int>(point.y()));
    
    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess *shortcutViewProcess = new QProcess(this);
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
    // qInfo() << "Shortcut preview finished";
}

void VNoteMainManager::showPrivacy()
{
    qDebug() << "Showing privacy policy";
    QString url = "";
    QLocale locale;
    QLocale::Country country = locale.country();
    bool isCommunityEdition = Utils::isCommunityEdition();
    if (country == QLocale::China) {
        if (isCommunityEdition) {
            url = "https://www.deepin.org/zh/agreement/privacy/";
        } else {
            url = "https://www.uniontech.com/agreement/privacy-cn";
        }
    } else {
        if (isCommunityEdition) {
            url = "https://www.deepin.org/en/agreement/privacy/";
        } else {
            url = "https://www.uniontech.com/agreement/privacy-en";
        }
    }
    qDebug() << "Opening privacy URL:" << url;
    QDesktopServices::openUrl(url);
    qInfo() << "Privacy policy shown";
}

void VNoteMainManager::resumeVoicePlayer()
{
    qInfo() << "Resuming voice player";
    JsContent::instance()->jsCallPlayVoice("", true);
    qInfo() << "Voice player resumed";
}

void VNoteMainManager::forceExit(bool needWait)
{
    qDebug() << "Force exiting application, needWait:" << needWait;
    if (needWait) {
        QTimer::singleShot(2000, this, [=]{
            m_eventloop.quit();
        });
        m_eventloop.exec();
    }
    VTextSpeechAndTrManager::instance()->onStopTextToSpeech();
    QApplication::exit(0);
    _Exit(0);
    qInfo() << "Force exit completed";
}

bool VNoteMainManager::isVoiceToText()
{
    qInfo() << "Checking voice to text status";
    bool result = OpsStateInterface::instance()->isVoice2Text();
    qDebug() << "Voice to text status:" << result;
    return result;
}

QString VNoteMainManager::getSavedTextPath()
{
    qInfo() << "Getting saved text path";
    QString savedPath = setting::instance()->getOption(VNOTE_EXPORT_TEXT_PATH_KEY).toString();
    qDebug() << "Retrieved saved path (unified):" << savedPath;
    return savedPath;
}

QString VNoteMainManager::getSavedVoicePath()
{
    qInfo() << "Getting saved voice path";
    QString savedPath = setting::instance()->getOption(VNOTE_EXPORT_TEXT_PATH_KEY).toString();
    qDebug() << "Retrieved saved path (unified):" << savedPath;
    return savedPath;
}

void VNoteMainManager::saveUserSelectedPath(const QString &path, const SaveAsType type)
{
    qDebug() << "Saving user selected path:" << path << "for type:" << type;
    
    QString dirPath = path;
    QFileInfo pathInfo(path);
    if (pathInfo.isFile() || (!pathInfo.exists() && !path.endsWith('/') && path.contains('.'))) {
        dirPath = pathInfo.absolutePath();
        qDebug() << "Extracted directory path:" << dirPath << "from file path:" << path;
    }
    
    setting::instance()->setOption(VNOTE_EXPORT_TEXT_PATH_KEY, dirPath);
    qDebug() << "Saved unified export directory to settings:" << dirPath;
}

int VNoteMainManager::currentNoteId() const
{
    return m_currentNoteId;
}

void VNoteMainManager::insertVoiceTextToNote(int noteId, const QString &voiceId, const QString &text)
{
    qInfo() << "insertVoiceTextToNote called, noteId:" << noteId 
            << "voiceId:" << voiceId << "text length:" << text.length();

    VNoteItem *note = getNoteById(noteId);
    if (!note) {
        qWarning() << "Note not found:" << noteId;
        return;
    }

    QString html = note->htmlCode;
    if (html.isEmpty()) {
        qWarning() << "Note htmlCode is empty:" << noteId;
        return;
    }

    // 在 HTML 中查找对应的语音元素
    // jsonKey 属性中包含 voiceId，需要找到并更新 text 字段
    // 格式如：jsonKey="{&quot;voiceId&quot;:&quot;uuid&quot;, ...}"

    // 查找包含该 voiceId 的 jsonKey
    // 由于 HTML 中的 JSON 是经过转义的，需要处理 &quot;
    int searchPos = 0;
    bool found = false;

    while (true) {
        int pos = html.indexOf("jsonKey=", searchPos);
        if (pos == -1) break;

        // 找到 jsonKey 的值
        int valueStart = html.indexOf('"', pos + 8);
        if (valueStart == -1) break;
        int valueEnd = html.indexOf('"', valueStart + 1);
        if (valueEnd == -1) break;

        QString jsonKeyValue = html.mid(valueStart + 1, valueEnd - valueStart - 1);

        // 检查是否包含目标 voiceId
        if (jsonKeyValue.contains(voiceId)) {
            // 解码 HTML 实体
            QString decodedJson = jsonKeyValue;
            decodedJson.replace("&quot;", "\"");
            decodedJson.replace("&amp;", "&");

            // 解析 JSON
            QJsonDocument doc = QJsonDocument::fromJson(decodedJson.toUtf8());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();

                // 验证 voiceId 匹配
                QString storedVoiceId = obj.value("voiceId").toString();
                if (storedVoiceId == voiceId) {
                    // 添加或更新 text 字段
                    obj["text"] = text;

                    // 编码回 HTML（先替换 & 再替换 "）
                    QString properJson = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                    properJson.replace("&", "&amp;");
                    properJson.replace("\"", "&quot;");

                    // 替换原来的 jsonKey 值
                    html.replace(valueStart + 1, valueEnd - valueStart - 1, properJson);

                    qInfo() << "Updated jsonKey with voice text for note:" << noteId << "voiceId:" << voiceId;
                    found = true;
                    break;
                }
            }
        }
        searchPos = valueEnd + 1;
    }

    if (!found) {
        qWarning() << "Voice element not found in note HTML, voiceId:" << voiceId;
        return;
    }

    // 保存到数据库
    note->htmlCode = html;
    note->modifyTime = QDateTime::currentDateTime();

    VNoteItemOper noteOper(note);
    if (noteOper.updateNote()) {
        qInfo() << "Voice text saved to note:" << noteId;
        emit noteDataUpdated(noteId);
    } else {
        qWarning() << "Failed to save voice text to note:" << noteId;
    }
}
