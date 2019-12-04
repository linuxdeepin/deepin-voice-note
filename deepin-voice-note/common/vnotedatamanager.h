#ifndef VNOTEDATAMANAGER_H
#define VNOTEDATAMANAGER_H

#include "datatypedef.h"

#include <QObject>

class LoadFolderWorker;

class VNoteDataManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDataManager(QObject *parent = nullptr);

    static VNoteDataManager* instance();


    //
    VNOTE_FOLDERS_MAP* getNoteFolders();

    VNOTE_ITEMS_MAP* getAllNotesInFolder(qint64 folder);

    void reqNoteFolders();
    void reqNoteItems(qint64 folder);
signals:
    void onNoteFoldersLoaded();
    void onNoteItemsLoaded();

public slots:
private:
    QScopedPointer<VNOTE_FOLDERS_MAP> m_qspNoteFoldersMap;

    LoadFolderWorker* m_pForldesLoadthread {nullptr};

    static VNoteDataManager* _instance;
};

#endif // VNOTEDATAMANAGER_H
