#ifndef VNOTEFOLDEROPER_H
#define VNOTEFOLDEROPER_H

#include "common/datatypedef.h"

#include <QPixmap>

class VNoteFolderOper
{
public:
    explicit VNoteFolderOper(VNoteFolder* folder = nullptr);

    inline bool        isNoteItemLoaded();
    VNOTE_FOLDERS_MAP* loadVNoteFolders();

    VNoteFolder* addFolder(VNoteFolder& folder);
    VNoteFolder* getFolder(qint64 folderId);
    qint32 getFoldersCount();
    qint32 getNotesCount(qint64 folderId);
    qint32 getNotesCount();

    QString getDefaultFolderName();
    qint32 getDefaultIcon();
    QPixmap getDefaultIcon(qint32 index, IconsType type);

    bool deleteVNoteFolder(qint64 folderId);
    bool deleteVNoteFolder(VNoteFolder* folder);
    bool renameVNoteFolder(QString folderName);

protected:
    VNoteFolder* m_folder {nullptr};
};

#endif // VNOTEFOLDEROPER_H
