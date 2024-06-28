// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatypedef.h"
#include "vnoteforlder.h"
#include "vnoteitem.h"
#include "vnoteitemoper.h"
#include "VNoteMainManager.h"
#include "jscontent.h"
#include "webrichetextmanager.h"

#include <QQmlApplicationEngine>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

VNoteMainManager::VNoteMainManager()
{

}

//写一个单例模式的类
VNoteMainManager *VNoteMainManager::instance()
{
    static VNoteMainManager instance;
    return &instance;
}

void VNoteMainManager::initNote()
{
    m_richTextManager = new WebRichTextManager();
    initData();
    initConnections();
}

void VNoteMainManager::initData()
{
    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    VNoteDataManager::instance()->reqNoteItems();
}

void VNoteMainManager::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onAllDatasReady,
        this, &VNoteMainManager::onVNoteFoldersLoaded);
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

void VNoteMainManager::initQMLRegister()
{
    qmlRegisterSingletonType<VNoteMainManager>("VNote", 1, 0, "VNoteMainManager", mainManager_provider);
    qmlRegisterSingletonType<JsContent>("VNote", 1, 0, "Webobj", jsContent_provider);
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

    //If have folders show note view,else show
    //default home page
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
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    // QVariant value = setting::instance()->getOption(VNOTE_FOLDER_SORT);
    // QStringList tmpsortFolders = value.toString().split(",");

    int folderCount = 0;

    if (folders) {
        folders->lock.lockForRead();

        QList<QVariantMap> foldersDataList;
        foreach (VNoteFolder *folder, folders->folders) {
            QVariantMap data;
            data.insert("name", folder->name);
            data.insert("notesCount", folder->notesCount);
            data.insert("icon", folder->UI.icon);
            data.insert("grayIcon", folder->UI.grayIcon);

            foldersDataList.append(data);
        }

        folders->lock.unlock();
        emit finishedFolderLoad(foldersDataList);
    }

    return folderCount;
}

void VNoteMainManager::vNoteFloderChanged(const int &index)
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        folders->lock.lockForRead();

        VNoteFolder *folder = folders->folders.value(index+1);
        m_currentFolderIndex = folder->id;
        if (!loadNotes(folder)) {
        }

        folders->lock.unlock();
    }
}

void VNoteMainManager::vNoteChanged(const int &index)
{
    VNoteItem *data = m_noteItems.at(index);
    m_richTextManager->initData(data, "");
    // if (data == nullptr || stateOperation->isSearching()) {
    //     m_recordBar->setVisible(false);
    // } else {
    //     m_recordBar->setVisible(true);
    // }
    //多选笔记时不更新详情页内容
    // if (!m_middleView->isMultipleSelected()) {
    //     m_richTextEdit->initData(data, m_searchKey, m_rightViewHasFouse);
    // }
    // //没有数据，插入图片按钮禁用
    // m_imgInsert->setDisabled(nullptr == data);
    // m_rightViewHasFouse = false;
}

/**
 * @brief VNoteMainManager::loadNotes
 * @param folder
 * @return 记事项数量
 */
int VNoteMainManager::loadNotes(VNoteFolder *folder)
{
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
                data.insert("name", it->noteTitle);
                data.insert("time", it->modifyTime);
                notesDataList.append(data);
                m_noteItems.append(it);
                notesCount++;
            }
            notes->lock.unlock();

            //Sort the view & set focus on first item
            // m_middleView->onNoteChanged();
            // m_middleView->setCurrentIndex(0);
        }
        emit updateNotes(notesDataList);
    }
    return notesCount;
}

void VNoteMainManager::createNote()
{
    if (m_currentFolderIndex == -1)
        return;
    VNoteItem tmpNote;
    tmpNote.folderId = m_currentFolderIndex;
    tmpNote.noteType = VNoteItem::VNT_Text;
    tmpNote.htmlCode = "<p><br></p>";
    VNoteItemOper noteOper;
    //Get default note name in the folder
    tmpNote.noteTitle = noteOper.getDefaultNoteName(tmpNote.folderId);

    VNoteItem *newNote = noteOper.addNote(tmpNote);

    //Refresh the notes count of folder
    // m_leftView->update(m_leftView->currentIndex());
    //解绑详情页绑定的笔记数据
    // m_richTextManager->unboundCurrentNoteData();
    // m_middleView->addRowAtHead(newNote);
    m_noteItems.append(newNote);
    QVariantMap data;
    data.insert("name", newNote->noteTitle);
    data.insert("time", newNote->modifyTime);
    emit addNoteAtHead(data);
}

void VNoteMainManager::deleteNote(const QList<int> &index)
{
    // 删除之前清空JS详情页内容
    m_richTextManager->clearJSContent();
    qWarning() << "deleteNote index:" << index;

    QList<VNoteItem*> noteDataList;
    for (int i = 0; i < index.size(); i++) {
        VNoteItem *note = m_noteItems.at(index.at(i));
        noteDataList.append(note);
        m_noteItems.removeAt(index.at(i));
    }

    if (noteDataList.size()) {
        //删除笔记之前先解除详情页绑定的笔记数据
        // m_richTextEdit->unboundCurrentNoteData();
        for (auto noteData : noteDataList) {
            VNoteItemOper noteOper(noteData);
            noteOper.deleteNote();
        }
        //Refresh the middle view
        // if (m_middleView->rowCount() <= 0 && stateOperation->isSearching()) {
        //     m_middleView->setVisibleEmptySearch(true);
        // }
    }
}