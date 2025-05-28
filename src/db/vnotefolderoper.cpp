// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnotefolderoper.h"
#include "common/utils.h"
#include "common/vnoteforlder.h"
#include "common/vnotedatamanager.h"
#include "db/vnotedbmanager.h"
#include "db/dbvisitor.h"
#include "globaldef.h"

#include <QVariant>
#include <QObject>
#include <QDateTime>
#include <QRandomGenerator>

#include <DApplication>

DWIDGET_USE_NAMESPACE

/**
 * @brief VNoteFolderOper::VNoteFolderOper
 * @param folder
 */
VNoteFolderOper::VNoteFolderOper(VNoteFolder *folder)
    : m_folder(folder)
{
}

/**
 * @brief VNoteFolderOper::isNoteItemLoaded
 * @return true 数据加载完成
 */
bool VNoteFolderOper::isNoteItemLoaded()
{
    return (m_folder) ? m_folder->fIsDataLoaded : false;
}

/**
 * @brief VNoteFolderOper::deleteVNoteFolder
 * @param folderId
 * @return true 成功
 */
bool VNoteFolderOper::deleteVNoteFolder(qint64 folderId)
{
    qDebug() << "Deleting folder with ID:" << folderId;
    bool delOK = false;

    DelFolderDbVisitor delFolderVisitor(
        VNoteDbManager::instance()->getVNoteDb(), &folderId, nullptr);

    if (VNoteDbManager::instance()->deleteData(&delFolderVisitor)) {
        delOK = true;
        QScopedPointer<VNoteFolder> release(VNoteDataManager::instance()->delFolder(folderId));
        qInfo() << "Successfully deleted folder:" << folderId;
    } else {
        qWarning() << "Failed to delete folder:" << folderId;
    }

    return delOK;
}

/**
 * @brief VNoteFolderOper::deleteVNoteFolder
 * @param folder
 * @return true 成功
 */
bool VNoteFolderOper::deleteVNoteFolder(VNoteFolder *folder)
{
    qDebug() << "Deleting folder:" << (folder ? folder->name : "null");
    bool delOK = true;
    if (nullptr != folder) {
        delOK = deleteVNoteFolder(folder->id);
    } else {
        qWarning() << "Cannot delete null folder";
    }

    return delOK;
}

/**
 * @brief VNoteFolderOper::renameVNoteFolder
 * @param folderName
 * @return true 成功
 */
bool VNoteFolderOper::renameVNoteFolder(const QString &folderName)
{
    qDebug() << "Renaming folder to:" << folderName;
    bool isUpdateOK = true;

    if (nullptr != m_folder) {
        QString oldFolderName = m_folder->name;
        QDateTime oldModifyTime = m_folder->modifyTime;

        m_folder->name = folderName;
        m_folder->modifyTime = QDateTime::currentDateTime();

        RenameFolderDbVisitor renameFolderVisitor(VNoteDbManager::instance()->getVNoteDb(), m_folder, nullptr);

        if (Q_UNLIKELY(!VNoteDbManager::instance()->updateData(&renameFolderVisitor))) {
            qWarning() << "Failed to rename folder from" << oldFolderName << "to" << folderName;
            m_folder->name = oldFolderName;
            m_folder->modifyTime = oldModifyTime;
            isUpdateOK = false;
        } else {
            qInfo() << "Successfully renamed folder from" << oldFolderName << "to" << folderName;
        }
    } else {
        qWarning() << "Cannot rename null folder";
        isUpdateOK = false;
    }

    return isUpdateOK;
}

/**
 * @brief VNoteFolderOper::loadVNoteFolders
 * @return 所有记事本数据
 */
VNOTE_FOLDERS_MAP *VNoteFolderOper::loadVNoteFolders()
{
    qDebug() << "Loading all note folders";
    VNOTE_FOLDERS_MAP *foldersMap = new VNOTE_FOLDERS_MAP();

    //DataManager should set autoRelease flag
    foldersMap->autoRelease = true;

    FolderQryDbVisitor folderVisitor(VNoteDbManager::instance()->getVNoteDb(), nullptr, foldersMap);

    if (!VNoteDbManager::instance()->queryData(&folderVisitor)) {
        qCritical() << "Failed to query folders";
    } else {
        qInfo() << "Successfully loaded" << foldersMap->folders.size() << "folders";
    }

    return foldersMap;
}

/**
 * @brief VNoteFolderOper::addFolder
 * @param folder
 * @return 数据处理后完整记事本数据
 */
VNoteFolder *VNoteFolderOper::addFolder(VNoteFolder &folder)
{
    qDebug() << "Adding new folder:" << folder.name;
    
    //Initialize
    folder.defaultIcon = getDefaultIcon();
    qDebug() << "Using default icon index:" << folder.defaultIcon;

    VNoteFolder *newFolder = new VNoteFolder();

    AddFolderDbVisitor addFolderVisitor(VNoteDbManager::instance()->getVNoteDb(), &folder, newFolder);

    if (VNoteDbManager::instance()->insertData(&addFolderVisitor)) {
        VNoteDataManager::instance()->addFolder(newFolder);

        qInfo() << "Successfully added new folder:"
                << "ID:" << newFolder->id
                << "Name:" << newFolder->name
                << "Create time:" << newFolder->createTime
                << "Modify time:" << newFolder->modifyTime;
    } else {
        qCritical() << "Failed to add folder:"
                    << "ID:" << newFolder->id
                    << "Name:" << newFolder->name
                    << "Create time:" << newFolder->createTime
                    << "Modify time:" << newFolder->modifyTime;

        QScopedPointer<VNoteFolder> autoRelease(newFolder);
        newFolder = nullptr;
    }

    return newFolder;
}

/**
 * @brief VNoteFolderOper::getFolder
 * @param folderId
 * @return 记事本数据
 */
VNoteFolder *VNoteFolderOper::getFolder(qint64 folderId)
{
    VNoteFolder *folder = VNoteDataManager::instance()->getFolder(folderId);

    return folder;
}

/**
 * @brief VNoteFolderOper::getFoldersCount
 * @return 记事本数量
 */
qint32 VNoteFolderOper::getFoldersCount()
{
    return VNoteDataManager::instance()->folderCount();
}

/**
 * @brief VNoteFolderOper::getNotesCount
 * @param folderId
 * @return 记事项数量
 */
qint32 VNoteFolderOper::getNotesCount(qint64 folderId)
{
    qDebug() << "Getting notes count for folder:" << folderId;
    VNOTE_ITEMS_MAP *notesInFollder = VNoteDataManager::instance()->getFolderNotes(folderId);

    qint32 notesCount = 0;

    if (nullptr != notesInFollder) {
        notesCount = notesInFollder->folderNotes.size();
        qDebug() << "Found" << notesCount << "notes in folder:" << folderId;
    } else {
        qDebug() << "No notes found in folder:" << folderId;
    }

    return notesCount;
}

/**
 * @brief VNoteFolderOper::getNotesCount
 * @return 记事项数量
 */
qint32 VNoteFolderOper::getNotesCount()
{
    qint32 notesCount = 0;

    if (m_folder != nullptr) {
        notesCount = getNotesCount(m_folder->id);
    }

    return notesCount;
}

/**
 * @brief VNoteFolderOper::getDefaultFolderName
 * @return 默认记事本名称
 */
QString VNoteFolderOper::getDefaultFolderName()
{
    //TODO:
    //    The default folder is auto-increment, and
    //may be separate data for different category in future.
    //We query the max id every time now, need optimize when
    //category feature is added.
    QString defaultFolderName = DApplication::translate("DefaultName", "Notebook");

    qint64 foldersCount = VNoteDataManager::instance()->folderCount();
    MaxIdFolderDbVisitor folderIdVisitor(VNoteDbManager::instance()->getVNoteDb(), nullptr, &foldersCount);

    //Need reset the folder table id if data are empty.
    if (foldersCount == 0) {
        qDebug() << "No existing folders, will reset folder table ID";
        folderIdVisitor.extraData().data.flag = true;
    }

    if (VNoteDbManager::instance()->queryData(&folderIdVisitor)) {
        defaultFolderName += QString("%1").arg(foldersCount + 1);
        qDebug() << "Generated default folder name:" << defaultFolderName;
    } else {
        qWarning() << "Failed to generate default folder name";
    }

    return defaultFolderName;
}

/**
 * @brief VNoteFolderOper::getDefaultIcon
 * @return 图标索引
 */
qint32 VNoteFolderOper::getDefaultIcon()
{
    const int defalutIconCnt = 10;
    return (QRandomGenerator::global()->bounded(0,defalutIconCnt));
}

/**
 * @brief VNoteFolderOper::getDefaultIcon
 * @param index
 * @param type
 * @return 图标
 */
QPixmap VNoteFolderOper::getDefaultIcon(qint32 index, IconsType type)
{
    return VNoteDataManager::instance()->getDefaultIcon(index, type);
}
