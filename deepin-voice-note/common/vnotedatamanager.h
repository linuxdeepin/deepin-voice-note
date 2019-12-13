#ifndef VNOTEDATAMANAGER_H
#define VNOTEDATAMANAGER_H

#include "datatypedef.h"

#include <QObject>

class LoadFolderWorker;
class LoadNoteItemsWorker;
class VNoteFolderOper;
class VNoteItemOper;
class FolderQryDbVisitor;

class VNoteDataManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDataManager(QObject *parent = nullptr);

    static VNoteDataManager* instance();


    //
    VNOTE_FOLDERS_MAP* getNoteFolders();

    VNOTE_ALL_NOTES_MAP* getAllNotesInFolder();

    void reqNoteDefIcons();
    void reqNoteFolders();
    void reqNoteItems();
signals:
    void onNoteFoldersLoaded();
    void onNoteItemsLoaded();

public slots:
protected:
    VNoteFolder* addFolder(VNoteFolder* folder);
    VNoteFolder* getFolder(qint64 folderId);
    VNoteFolder* delFolder(qint64 folderId);
    qint32       folderCount();

    VNoteItem* addNote(VNoteItem* note);
    VNoteItem* getNote(qint64 folderId, qint32 noteId);
    VNoteItem* delNote(qint64 folderId, qint32 noteId);
    /*
     *Para:   folder Id
     *Return: All nots in the folder
     * */
    VNOTE_ITEMS_MAP* getFolderNotes(qint64 folderId);

    QImage getDefaultIcon(qint32 index);
private:
    QScopedPointer<VNOTE_FOLDERS_MAP> m_qspNoteFoldersMap;
    QScopedPointer<VNOTE_ALL_NOTES_MAP> m_qspAllNotesMap;

    LoadFolderWorker    *m_pForldesLoadThread {nullptr};
    LoadNoteItemsWorker *m_pNotesLoadThread {nullptr};

    static VNoteDataManager* _instance;

    static QVector<QImage> m_defaultIcons;
    static QReadWriteLock m_iconLock;

    friend class LoadIconsWorker;
    friend class VNoteFolderOper;
    friend class VNoteItemOper;
    friend class FolderQryDbVisitor;
};

#endif // VNOTEDATAMANAGER_H
