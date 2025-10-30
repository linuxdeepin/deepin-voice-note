// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
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
#include "audio/recording_curves.h"

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
#include <QProcess>
#include <QDesktopServices>
#include <QDebug>

#include <DSysInfo>

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
    connect(m_richTextManager, &WebRichTextManager::finishedUpdateNote, this, &VNoteMainManager::exitWithSave);
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
    QStringList tmpsortFolders;
    QString value = setting::instance()->getOption(VNOTE_FOLDER_SORT).toString();
    if (!value.isEmpty()) {
        tmpsortFolders = value.split(",");
        m_folderSort = tmpsortFolders;
        qDebug() << "Loaded folder sort order:" << tmpsortFolders;
    }

    int folderCount = 0;

    if (folders) {
        qInfo() << "folders is not nullptr";
        folders->lock.lockForRead();
        folderCount = folders->folders.size();
        qDebug() << "Found" << folderCount << "folders";

        int index = folderCount;
        QList<QVariantMap> foldersDataList;
        foreach (VNoteFolder *folder, folders->folders) {
            int tmpIndexCount = tmpsortFolders.indexOf(QString::number(folder->id));
            QVariantMap data;
            data.insert(FOLDER_NAME_KEY, folder->name);
            data.insert(FOLDER_COUNT_KEY, QString::number(folder->getNotesCount()));
            data.insert(FOLDER_ICON_KEY, QString::number(folder->defaultIcon));
            data.insert(FOLDER_ID_KEY, folder->id);
            if (tmpIndexCount != -1) {
                folder->sortNumber = tmpIndexCount;
            } else {
                folder->sortNumber = index;
                m_folderSort.append(QString::number(index));
            }
            data.insert(FOLDER_SORT_KEY, folder->sortNumber);

            foldersDataList.append(data);
            index--;
        }

        folders->lock.unlock();
        // 将foldersDataList按照sortNumber排序
        std::sort(foldersDataList.begin(), foldersDataList.end(),
                  [](const QVariantMap &a, const QVariantMap &b) {
                      return a[FOLDER_SORT_KEY].toInt() < b[FOLDER_SORT_KEY].toInt();
                  });
        emit finishedFolderLoad(foldersDataList);
        qDebug() << "Folder data sorted and emitted";
    }

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

void VNoteMainManager::vNoteDeleteFolder(const int &index)
{
    qDebug() << "Deleting folder at index:" << index;
    // 录音或播放中禁止删除
    if (VoiceRecoderHandler::instance()->getRecoderType() == VoiceRecoderHandler::Recording
        || OpsStateInterface::instance()->isPlaying()) {
        qWarning() << "Cannot delete folder while recording or playing";
        return;
    }
    VNoteFolder *folder = getFloderByIndex(index);

    int listIndex = m_folderSort.indexOf(QString::number(folder->id));
    if (-1 != listIndex) {
        m_folderSort.removeAt(listIndex);
        qDebug() << "Removed folder from sort list";
    }

    setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
    if (folder) {
        VNoteFolderOper folderOper(folder);
        folderOper.deleteVNoteFolder(folder);
        qDebug() << "Folder deleted successfully";
    } else {
        qWarning() << "Invalid folder index for deletion:" << index;
    }
    qInfo() << "Folder deletion finished";
}

void VNoteMainManager::vNoteChanged(const int &index)
{
    qInfo() << "Changing to note index:" << index;
    if (index < 0) {
        qWarning() << "Invalid note index:" << index;
        return;
    }
    qDebug() << "Changing to note index:" << index;
    m_currentNoteId = index;
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

struct NoteCompare {
    bool operator()(const QVariantMap &a, const QVariantMap &b) const {
        // 比较 isTop 字段
        bool aIsTop = a.value(NOTE_ISTOP_KEY).toBool();
        bool bIsTop = b.value(NOTE_ISTOP_KEY).toBool();

        if (aIsTop != bIsTop) {
            // 如果 isTop 不相同，则 isTop 为 true 的排在前面
            return aIsTop > bIsTop;
        } else {
            // 如果 isTop 相同，则根据 modifyTime 排序
            QDateTime aModifyTime = QDateTime::fromString(a.value(NOTE_MODIFY_TIME_KEY).toString(), "yyyy-MM-dd hh:mm:ss");
            QDateTime bModifyTime = QDateTime::fromString(b.value(NOTE_MODIFY_TIME_KEY).toString(), "yyyy-MM-dd hh:mm:ss");

            // 对于相同 isTop 的笔记，按照最近修改时间降序排列
            return aModifyTime > bModifyTime;
        }
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
    qInfo() << "Creating new note";
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

    if (m_currentFolderIndex == -1) {
        qWarning() << "Cannot create note: No current folder selected";
        return;
    }
    VNoteFolder *currentFolder = getFloderById(m_currentFolderIndex);
    if (currentFolder == nullptr) {
        qWarning() << "Cannot create note: Current folder not found for ID:" << m_currentFolderIndex;
        return;
    }
    qDebug() << "Creating new note in folder ID:" << m_currentFolderIndex;
    VNoteItem tmpNote;
    tmpNote.folderId = m_currentFolderIndex;
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
    m_currentNoteId = newNote->noteId;

    m_noteItems.append(newNote);

    QVariantMap data;
    data.insert(NOTE_NAME_KEY, newNote->noteTitle);
    data.insert(NOTE_TIME_KEY, Utils::convertDateTime(newNote->modifyTime));
    data.insert(NOTE_MODIFY_TIME_KEY, newNote->modifyTime.toString("yyyy-MM-dd hh:mm:ss"));
    data.insert(NOTE_ISTOP_KEY, newNote->isTop);
    data.insert(NOTE_FOLDER_ICON_KEY, QString::number(currentFolder->defaultIcon));
    data.insert(NOTE_FOLDER_NAME_KEY, currentFolder->name);
    data.insert(NOTE_ID_KEY, newNote->noteId);

    emit addNoteAtHead(data);
    m_richTextManager->initData(newNote, "");
    m_richTextManager->clearJSContent();
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
            if (folder->id == id) {
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
    for (int i = 0; i < m_folderSort.size(); i++) {
        int tmpIndex = m_folderSort.at(i).toInt();
        VNoteFolder *folder = folders->folders.value(tmpIndex);
        if (folder->id == id)
            return i;
    }
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

void VNoteMainManager::deleteNote(const QList<int> &index)
{
    // 删除之前清空JS详情页内容
    qDebug() << "Deleting" << index.size() << "notes";
    // 录音或播放中禁止删除
    if (VoiceRecoderHandler::instance()->getRecoderType() == VoiceRecoderHandler::Recording
        || OpsStateInterface::instance()->isPlaying()) {
        qWarning() << "Cannot delete note while recording or playing";
        return;
    }
    m_richTextManager->clearJSContent();
    QList<VNoteItem *> noteDataList;
    for (int i = 0; i < index.size(); i++) {
        VNoteItem *note = getNoteById(index.at(i));
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
    }
    qInfo() << "Note deletion finished";
}

void VNoteMainManager::moveNotes(const QVariantList &index, const int &folderIndex)
{
    qDebug() << "Moving" << index.size() << "notes to folder index:" << folderIndex;
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (index.isEmpty() || folderIndex < 0 || folderIndex >= folders->folders.size()) {
        qWarning() << "Invalid move parameters";
        return;
    }
    VNoteFolder *folder = getFloderByIndex(folderIndex);
    if (!index.at(0).isValid()) {
        qWarning() << "Invalid note index";
        return;
    }
    VNoteItem *item = getNoteById(index.at(0).toInt());
    if (folder == nullptr || item == nullptr || item->folderId == folder->id) {
        qWarning() << "Invalid move operation";
        return;
    }

    VNoteItemOper noteOper;
    VNOTE_ITEMS_MAP *srcNotes = noteOper.getFolderNotes(item->folderId);
    VNOTE_ITEMS_MAP *destNotes = noteOper.getFolderNotes(folder->id);
    int srcIndex = getFloderIndexById(item->folderId);
    foreach (auto i, index) {
        VNoteItem *note = getNoteById(i.toInt());
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
    }
    folder->maxNoteIdRef() += index.size();
    qDebug() << "Notes moved successfully";
    emit moveFinished(index, srcIndex, folderIndex);
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
    VNoteItem *note = getNoteById(m_currentNoteId);
    if (!note) {
        qWarning() << "onNoteChanged: current note not found, id=" << m_currentNoteId << ", skip";
        return;
    }
    note->modifyTime = QDateTime::currentDateTime();
    emit updateEditNote(m_currentNoteId, Utils::convertDateTime(note->modifyTime));
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

void VNoteMainManager::exitWithSave()
{
    qInfo() << "Exiting with save";
    if (m_eventloop.isRunning())
        m_eventloop.quit();
    qInfo() << "Exit with save finished";
}

bool VNoteMainManager::getTop()
{
    qInfo() << "Getting top status:" << m_currentHasTop;
    return m_currentHasTop;
}

void VNoteMainManager::updateSort(const int &src, const int &dst)
{
    qInfo() << "Updating sort, src:" << src << "dst:" << dst;
    QString tmp = m_folderSort.at(src);
    m_folderSort.removeAt(src);
    m_folderSort.insert(dst, tmp);
    setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
    qInfo() << "Sort update finished";
}

void VNoteMainManager::renameFolder(const int &index, const QString &name)
{
    qDebug() << "Renaming folder at index" << index << "to:" << name;
    VNoteFolder *folder = getFloderByIndex(index);
    if (folder && name != folder->name) {
        VNoteFolderOper folderOper(folder);
        folderOper.renameVNoteFolder(name);
        qDebug() << "Folder renamed successfully";
    } else {
        qWarning() << "Invalid folder rename operation";
    }
    qInfo() << "Folder rename finished";
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
    m_richTextManager->onUpdateNoteWithResult(getNoteById(m_currentNoteId), result);
    qInfo() << "Note update with result finished";
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
        QString localPath = path.toLocalFile();
        QFileInfo fileInfo(localPath);
        QString suffix = fileInfo.suffix();
        if (!(suffix == "jpg" || suffix == "png" || suffix == "bmp")) {
            qWarning() << "Unsupported image format:" << suffix;
            continue;
        }
        //创建文件路径
        QString newPath = QString("%1/%2_%3.%4").arg(dirPath).arg(date).arg(++count).arg(suffix);
        if (QFile::copy(QUrl(path).toLocalFile(), newPath)) {
            paths.push_back(newPath);
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
    foreach (auto id, index) {
        int noteIndex = id.toInt();
        VNoteItem *item = getNoteById(noteIndex);
        if (item->haveVoice()) {
            ActionManager::instance()->enableAction(ActionManager::NoteSaveVoice, true);
            return;
        }
    }
    ActionManager::instance()->enableAction(ActionManager::NoteSaveVoice, false);
    qInfo() << "Note voice check finished";
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
    // qInfo() << "Clearing search";
    m_searchText = "";
    // qInfo() << "Search cleared";
}

bool VNoteMainManager::isInSearchMode() const
{
    qInfo() << "Checking search mode:" << !m_searchText.isEmpty();
    return !m_searchText.isEmpty();
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
