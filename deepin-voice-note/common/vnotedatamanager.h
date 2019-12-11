#ifndef VNOTEDATAMANAGER_H
#define VNOTEDATAMANAGER_H

#include "datatypedef.h"

#include <QObject>

class LoadFolderWorker;
class VNoteFolderOper;
class VNoteItemOper;

class VNoteDataManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDataManager(QObject *parent = nullptr);

    static VNoteDataManager* instance();


    //
    VNOTE_FOLDERS_MAP* getNoteFolders();

    VNOTE_ITEMS_MAP* getAllNotesInFolder(qint64 folder);

    void reqNoteDefIcons();
    void reqNoteFolders();
    void reqNoteItems(qint64 folder);
signals:
    void onNoteFoldersLoaded();
    void onNoteItemsLoaded();

public slots:
protected:
    VNoteFolder* addFolder(VNoteFolder* folder);
    VNoteFolder* getFolder(qint64 folderId);
    VNoteFolder* delFolder(qint64 folderId);

    QImage getDefaultIcon(qint32 index);
private:
    QScopedPointer<VNOTE_FOLDERS_MAP> m_qspNoteFoldersMap;

    LoadFolderWorker* m_pForldesLoadthread {nullptr};

    static VNoteDataManager* _instance;

    static QVector<QImage> m_defaultIcons;
    static QReadWriteLock m_iconLock;

    friend class LoadIconsWorker;
    friend class VNoteFolderOper;
    friend class VNoteItemOper;
};

#endif // VNOTEDATAMANAGER_H
