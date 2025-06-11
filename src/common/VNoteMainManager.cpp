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
}

// 写一个单例模式的类
VNoteMainManager *VNoteMainManager::instance()
{
    static VNoteMainManager instance;
    return &instance;
}

void VNoteMainManager::initNote()
{
    m_richTextManager = new WebRichTextManager();
    initConnections();
    initData();
}

void VNoteMainManager::initData()
{
    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    QThread::msleep(200);
    VNoteDataManager::instance()->reqNoteItems();
}

void VNoteMainManager::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onAllDatasReady,
            this, &VNoteMainManager::onVNoteFoldersLoaded);
    connect(m_richTextManager, &WebRichTextManager::needUpdateNote, this, &VNoteMainManager::needUpdateNote);
    connect(m_richTextManager, &WebRichTextManager::noteTextChanged, this, &VNoteMainManager::onNoteChanged, Qt::QueuedConnection);
    connect(m_richTextManager, &WebRichTextManager::updateSearch, this, &VNoteMainManager::updateSearch);
    connect(m_richTextManager, &WebRichTextManager::scrollChange, this, &VNoteMainManager::scrollChange);
    connect(m_richTextManager, &WebRichTextManager::finishedUpdateNote, this, &VNoteMainManager::exitWithSave);
    connect(VoiceRecoderHandler::instance(), &VoiceRecoderHandler::finishedRecod, this, &VNoteMainManager::insertVoice);
}

QObject *jsContent_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return JsContent::instance();
}

QObject *mainManager_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return VNoteMainManager::instance();
}

QObject *actionManager_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return ActionManager::instance();
}

QObject *voiceRecoder_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return VoiceRecoderHandler::instance();
}

void VNoteMainManager::initQMLRegister()
{
    qmlRegisterSingletonType<VNoteMainManager>("VNote", 1, 0, "VNoteMainManager", mainManager_provider);
    qmlRegisterSingletonType<JsContent>("VNote", 1, 0, "Webobj", jsContent_provider);
    // 菜单项管理
    qmlRegisterSingletonType<ActionManager>("VNote", 1, 0, "ActionManager", actionManager_provider);

    // QML组件访问后端调用
    qmlRegisterType<WebEngineHandler>("VNote", 1, 0, "WebEngineHandler");
    qmlRegisterType<VNoteMessageDialogHandler>("VNote", 1, 0, "VNoteMessageDialogHandler");
    qmlRegisterSingletonType<VoiceRecoderHandler>("VNote", 1, 0, "VoiceRecoderHandler", voiceRecoder_provider);

    qmlRegisterType<RecordingCurves>("VNote", 1, 0, "RecordingCurves");
}

void VNoteMainManager::onVNoteFoldersLoaded()
{
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

    return folderCount;
}

void VNoteMainManager::vNoteFloderChanged(const int &index)
{
    qDebug() << "Changing to folder index:" << index;
    VNoteFolder *folder = getFloderByIndex(index);
    if (folder) {
        m_currentFolderIndex = folder->id;
        qDebug() << "Loading notes for folder ID:" << folder->id;
        if (!loadNotes(folder)) {
            qWarning() << "Failed to load notes for folder ID:" << folder->id;
        }
    } else {
        qWarning() << "Invalid folder index:" << index;
    }
}

void VNoteMainManager::vNoteCreateFolder()
{
    qDebug() << "Creating new folder";
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
        addFolderFinished(data);
    } else {
        qWarning() << "Failed to create new folder";
    }
}

void VNoteMainManager::vNoteDeleteFolder(const int &index)
{
    qDebug() << "Deleting folder at index:" << index;
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
}

void VNoteMainManager::vNoteChanged(const int &index)
{
    if (index < 0) {
        qWarning() << "Invalid note index:" << index;
        return;
    }
    qDebug() << "Changing to note index:" << index;
    m_currentNoteId = index;
    VNoteItem *data = getNoteById(m_currentNoteId);
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
    m_currentHasTop = 0;
    m_noteItems.clear();
    int notesCount = -1;
    if (folder) {
        notesCount = 0;
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(folder->id);
        QList<QVariantMap> notesDataList;
        if (notes) {
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
        if (!notesDataList.isEmpty()) {
            m_currentNoteId = notesDataList.first().value("noteId").toInt();
            vNoteChanged(m_currentNoteId);
        } else
            m_currentNoteId = -1;
        emit updateNotes(notesDataList, 0);
    }
    return notesCount;
}

void VNoteMainManager::insertVoice(const QString &path, qint64 size)
{
    m_richTextManager->insertVoiceItem(path, size);
}

void VNoteMainManager::createNote()
{
    if (m_currentFolderIndex == -1) {
        qWarning() << "Cannot create note: No current folder selected";
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
    m_currentNoteId = newNote->noteId;

    m_noteItems.append(newNote);

    QVariantMap data;
    data.insert(NOTE_NAME_KEY, newNote->noteTitle);
    data.insert(NOTE_TIME_KEY, Utils::convertDateTime(newNote->modifyTime));
    data.insert(NOTE_MODIFY_TIME_KEY, newNote->modifyTime.toString("yyyy-MM-dd hh:mm:ss"));
    data.insert(NOTE_ISTOP_KEY, newNote->isTop);
    VNoteFolder *folder = getFloderById(newNote->folderId);
    data.insert(NOTE_FOLDER_ICON_KEY, QString::number(folder->defaultIcon));
    data.insert(NOTE_FOLDER_NAME_KEY, folder->name);
    data.insert(NOTE_ID_KEY, newNote->noteId);

    emit addNoteAtHead(data);
    m_richTextManager->initData(newNote, "");
    m_richTextManager->clearJSContent();
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
    switch (type) {
    case Html:
        exportType = ExportNoteWorker::ExportHtml;
        defaultName = QUrl(path).fileName();
        break;
    case Text:
        exportType = ExportNoteWorker::ExportText;
        defaultName = QUrl(path).fileName();
        break;
    case Voice:
        exportType = ExportNoteWorker::ExportVoice;
        // 通过qt获取当前时间并转换为字符串
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
}

VNoteFolder *VNoteMainManager::getFloderByIndex(const int &index)
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    int tmpIndex = m_folderSort.at(index).toInt();
    if (folders) {
        folders->lock.lockForRead();

        VNoteFolder *folder = folders->folders.value(tmpIndex);
        folders->lock.unlock();
        return folder;
    }
    return nullptr;
}

VNoteFolder *VNoteMainManager::getFloderById(const int &id)
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
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
    foreach (auto item, m_noteItems) {
        if (item->noteId == id)
            return item;
    }
    return nullptr;
}

VNoteItem *VNoteMainManager::deleteNoteById(const int &id)
{
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
        qDebug() << "Processing deletion of" << noteDataList.size() << "notes";
        for (auto noteData : noteDataList) {
            VNoteItemOper noteOper(noteData);
            noteOper.deleteNote();
        }
    } else {
        qWarning() << "No notes to delete";
    }
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
}

void VNoteMainManager::updateTop(const int &id, const bool &top)
{
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
}

void VNoteMainManager::onExportFinished(int err)
{
    Q_UNUSED(err)
    // TODO:提示保存成功
}

void VNoteMainManager::onNoteChanged()
{
    VNoteItem *note = getNoteById(m_currentNoteId);
    note->modifyTime = QDateTime::currentDateTime();
    emit updateEditNote(m_currentNoteId, Utils::convertDateTime(note->modifyTime));
}

void VNoteMainManager::updateSearch()
{
    if (m_searchText.isEmpty()) {
        qDebug() << "Search text is empty, skipping update";
        return;
    }
    qDebug() << "Updating search with text:" << m_searchText;
    emit updateRichTextSearch(m_searchText);
}

void VNoteMainManager::exitWithSave()
{
    if (m_eventloop.isRunning())
        m_eventloop.quit();
}

bool VNoteMainManager::getTop()
{
    return m_currentHasTop;
}

void VNoteMainManager::updateSort(const int &src, const int &dst)
{
    QString tmp = m_folderSort.at(src);
    m_folderSort.removeAt(src);
    m_folderSort.insert(dst, tmp);
    setting::instance()->setOption(VNOTE_FOLDER_SORT, m_folderSort.join(","));
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
}

void VNoteMainManager::renameNote(const int &index, const QString &newName)
{
    qDebug() << "Renaming note at index" << index << "to:" << newName;
    VNoteItem *item = getNoteById(index);
    if (item && !newName.isEmpty() && newName != item->noteTitle) {
        VNoteItemOper noteOps(item);
        noteOps.modifyNoteTitle(newName);
        qDebug() << "Note renamed successfully";
    } else {
        qWarning() << "Invalid note rename operation";
    }
    onNoteChanged();
}

void VNoteMainManager::vNoteSearch(const QString &text)
{
    if (!text.isEmpty()) {
        if (m_searchText != text) {
            qDebug() << "Starting new search with text:" << text;
            m_searchText = text;
            loadSearchNotes(text);
        }
        updateSearch();
    } else {
        qDebug() << "Search text is empty";
    }
}

void VNoteMainManager::updateNoteWithResult(const QString &result)
{
    m_richTextManager->onUpdateNoteWithResult(getNoteById(m_currentNoteId), result);
}

int VNoteMainManager::loadSearchNotes(const QString &key)
{
    if (key.isEmpty()) {
        qWarning() << "Empty search key";
        return -1;
    }
    qDebug() << "Loading search notes for key:" << key;
    VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
    QList<QVariantMap> notesDataList;
    m_noteItems.clear();
    if (noteAll) {
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
            //TODO:有搜索结果
            emit searchFinished(notesDataList, key);
        }
    }
    return notesDataList.size();
}

int VNoteMainManager::loadAudioSource()
{
    return setting::instance()->getOption(VNOTE_AUDIO_SELECT).toInt();
}

void VNoteMainManager::changeAudioSource(const int &source)
{
    setting::instance()->setOption(VNOTE_AUDIO_SELECT, QVariant(source));
    VoiceRecoderHandler::instance()->changeMode(source);
}

void VNoteMainManager::insertImages(const QStringList &filePaths)
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
        QFileInfo fileInfo(path);
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
}

void VNoteMainManager::checkNoteVoice(const QVariantList &index)
{
    foreach (auto id, index) {
        int noteIndex = id.toInt();
        VNoteItem *item = getNoteById(noteIndex);
        if (item->haveVoice()) {
            ActionManager::instance()->enableAction(ActionManager::NoteSaveVoice, true);
            return;
        }
    }
    ActionManager::instance()->enableAction(ActionManager::NoteSaveVoice, false);
}

void VNoteMainManager::clearSearch()
{
    m_searchText = "";
}

void VNoteMainManager::preViewShortcut(const QPointF &point)
{
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
                                                         {DApplication::translate("Shortcuts", "Redo"), "Ctrl+Y"},
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

    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(point.x()) + "," + QString::number(point.y());
    shortcutString << param1 << param2;

    QProcess *shortcutViewProcess = new QProcess(this);
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

void VNoteMainManager::showPrivacy()
{
    qDebug() << "Showing privacy policy";
    QString url = "";
    QLocale locale;
    QLocale::Country country = locale.country();
    bool isCommunityEdition = DSysInfo::isCommunityEdition();
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
}

void VNoteMainManager::resumeVoicePlayer()
{
    JsContent::instance()->jsCallPlayVoice("", true);
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
}

bool VNoteMainManager::isVoiceToText()
{
    bool result = OpsStateInterface::instance()->isVoice2Text();
    qDebug() << "Voice to text status:" << result;
    return result;
}
