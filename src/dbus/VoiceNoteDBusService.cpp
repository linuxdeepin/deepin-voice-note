// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VoiceNoteDBusService.h"
#include "../common/VNoteMainManager.h"
#include "../common/vnotedatamanager.h"
#include "../common/vnoteforlder.h"
#include "../common/vnoteitem.h"
#include "../handler/voice_recoder_handler.h"
#include "../db/vnoteitemoper.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGuiApplication>
#include <QWindow>
#include <QThread>
#include <QDebug>

VoiceNoteDBusService::VoiceNoteDBusService(QObject *parent)
    : QObject(parent)
{
}

VoiceNoteDBusService::~VoiceNoteDBusService()
{
}

bool VoiceNoteDBusService::initDBusService()
{
    qInfo() << "Initializing D-Bus service";
    
    QDBusConnection connection = QDBusConnection::sessionBus();
    
    if (!connection.registerService("org.deepin.voicenote")) {
        qWarning() << "Failed to register D-Bus service:" << connection.lastError().message();
        return false;
    }
    
    if (!connection.registerObject("/org/deepin/voicenote", this, 
                                   QDBusConnection::ExportAllSlots)) {
        qWarning() << "Failed to register D-Bus object:" << connection.lastError().message();
        return false;
    }
    
    qInfo() << "  D-Bus service registered successfully";
    qInfo() << "  Service: org.deepin.voicenote";
    qInfo() << "  Object: /org/deepin/voicenote";
    
    return true;
}

QString VoiceNoteDBusService::GetNotesList()
{
    qInfo() << "D-Bus: GetNotesList called";
    
    QJsonObject root;
    
    // 获取所有文件夹
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (!folders) {
        qWarning() << "Failed to get folders";
        return "{}";
    }
    
    // 获取所有笔记
    VNOTE_ALL_NOTES_MAP *allNotes = VNoteDataManager::instance()->getAllNotesInFolder();
    if (!allNotes) {
        qWarning() << "Failed to get notes";
        return "{}";
    }
    
    // 遍历所有文件夹
    for (auto folderIt = folders->folders.begin(); folderIt != folders->folders.end(); ++folderIt) {
        VNoteFolder *folder = folderIt.value();
        if (!folder) continue;
        
        QJsonObject notesObj;
        
        // 查找该文件夹下的笔记
        auto notesIt = allNotes->notes.find(folder->id);
        if (notesIt != allNotes->notes.end()) {
            VNOTE_ITEMS_MAP *notes = notesIt.value();
            if (notes) {
                for (auto noteIt = notes->folderNotes.begin(); noteIt != notes->folderNotes.end(); ++noteIt) {
                    VNoteItem *note = noteIt.value();
                    if (!note) continue;
                    
                    // 只保存 noteId: title
                    notesObj[QString::number(note->noteId)] = note->noteTitle;
                }
            }
        }
        
        // 只在有笔记时添加文件夹
        if (!notesObj.isEmpty()) {
            root[QString::number(folder->id)] = notesObj;
        }
    }
    
    QString result = QJsonDocument(root).toJson(QJsonDocument::Compact);
    qInfo() << "D-Bus: Returning notes list with" << root.size() << "folders";
    
    return result;
}

bool VoiceNoteDBusService::RecordVoice(int folderId, int noteId)
{
    qInfo() << "D-Bus: RecordVoice called with folderId:" << folderId << "noteId:" << noteId;
    
    VNoteMainManager *manager = VNoteMainManager::instance();
    if (!manager) {
        qWarning() << "Failed to get VNoteMainManager instance";
        return false;
    }
    
    auto windows = qApp->topLevelWindows();
    if (!windows.isEmpty()) {
        QWindow *window = windows.first();
        window->raise();
        window->requestActivate();
        qInfo() << "  Window activated";
    }
    
    if (folderId == -1 || noteId == -1) {
        qInfo() << "  Creating new folder and note";
        
        manager->vNoteCreateFolder();
        QThread::msleep(200);
        
        manager->createNote();
        QThread::msleep(200);
        
        VoiceRecoderHandler::instance()->startRecoder();
        qInfo() << "Recording started in new note";
        return true;
    }
    
    // 3. 验证文件夹是否存在
    VNoteFolder *targetFolder = manager->getFloderById(folderId);
    if (!targetFolder) {
        qWarning() << "  Folder not found:" << folderId;
        return false;
    }
    
    // 4. 从数据库获取目标笔记
    VNoteItemOper noteOper;
    VNoteItem *targetNote = noteOper.getNote(folderId, noteId);
    if (!targetNote) {
        qWarning() << "  Note not found:" << noteId << "in folder:" << folderId;
        return false;
    }
    
    // 5. 切换到目标文件夹
    int folderIndex = manager->getFloderIndexById(folderId);
    if (folderIndex == -1) {
        qWarning() << "  Folder index not found for folder:" << folderId;
        return false;
    }
    
    qInfo() << "  Switching to folder:" << folderId << "(index:" << folderIndex << ")";
    manager->vNoteFloderChanged(folderIndex);
    QThread::msleep(200);
    
    // 6. 切换到目标笔记并更新UI选中状态
    qInfo() << "  Switching to note:" << noteId;
    manager->vNoteChangedWithUIUpdate(noteId);
    QThread::msleep(300); 
    
    // 7. 开始录音
    auto *recorder = VoiceRecoderHandler::instance();
    if (recorder->getRecoderType() != VoiceRecoderHandler::Idle) {
        qWarning() << "Cannot start recording: already recording or paused";
        return false;
    }
    
    recorder->startRecoder();
    qInfo() << "Recording started successfully";
    
    return true;
}

