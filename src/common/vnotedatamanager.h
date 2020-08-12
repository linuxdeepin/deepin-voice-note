/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef VNOTEDATAMANAGER_H
#define VNOTEDATAMANAGER_H

#include "datatypedef.h"

#include <QObject>

class LoadFolderWorker;
class LoadNoteItemsWorker;
class VNoteFolderOper;
class VNoteItemOper;
class FolderQryDbVisitor;
struct VNoteFolder;

class VNoteDataManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDataManager(QObject *parent = nullptr);

    static VNoteDataManager *instance();

    //
    VNOTE_FOLDERS_MAP *getNoteFolders();

    VNOTE_ALL_NOTES_MAP *getAllNotesInFolder();

    void reqNoteDefIcons();
    void reqNoteFolders();
    void reqNoteItems();
signals:
    void onNoteFoldersLoaded();
    void onNoteItemsLoaded();
    void onAllDatasReady();

public slots:
protected:
    VNoteFolder *addFolder(VNoteFolder *folder);
    VNoteFolder *getFolder(qint64 folderId);
    VNoteFolder *delFolder(qint64 folderId);
    qint32 folderCount();

    VNoteItem *addNote(VNoteItem *note);
    VNoteItem *getNote(qint64 folderId, qint32 noteId);
    VNoteItem *delNote(qint64 folderId, qint32 noteId);
    qint32 folderNotesCount(qint64 folderId);
    /*
     *Para:   folder Id
     *Return: All nots in the folder
     * */
    VNOTE_ITEMS_MAP *getFolderNotes(qint64 folderId);

    QPixmap getDefaultIcon(qint32 index, IconsType type);

private:
    QScopedPointer<VNOTE_FOLDERS_MAP> m_qspNoteFoldersMap;
    QScopedPointer<VNOTE_ALL_NOTES_MAP> m_qspAllNotesMap;

    LoadFolderWorker *m_pForldesLoadThread {nullptr};
    LoadNoteItemsWorker *m_pNotesLoadThread {nullptr};

    //State used to check whether data have been loaded
    // or not.
    enum DataState {
        DataNotLoaded = 0x0,
        FolderDataReady = (0x1),
        NotesDataReady = (0x1 << 1),
    };

    int m_fDataState = {DataNotLoaded};

    bool isAllDatasReady() const;

    static VNoteDataManager *_instance;

    static QVector<QPixmap> m_defaultIcons[IconsType::MaxIconsType];
    static QReadWriteLock m_iconLock;

    friend class LoadIconsWorker;
    friend class VNoteFolderOper;
    friend class VNoteItemOper;
    friend class FolderQryDbVisitor;
    friend struct VNoteFolder;
};

#endif // VNOTEDATAMANAGER_H
