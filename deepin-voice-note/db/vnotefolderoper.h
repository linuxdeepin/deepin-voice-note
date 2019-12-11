#ifndef VNOTEFOLDEROPER_H
#define VNOTEFOLDEROPER_H

#include "common/datatypedef.h"

#include <QImage>

class VNoteFolderOper
{
public:
    explicit VNoteFolderOper(VNoteFolder* folder = nullptr);

    inline bool        isNoteItemLoaded();
    VNOTE_FOLDERS_MAP* loadVNoteFolders();

    VNoteFolder* addFolder(VNoteFolder& folder);
    VNoteFolder* getFolder(qint64 folderId);

    QString getDefaultFolderName();
    qint32 getDefaultIcon();
    QImage getDefaultIcon(qint32 index);

    bool deleteVNoteFolder(qint64 folderId);
    bool renameVNoteFolder(QString folderName);

protected:
    VNoteFolder* m_folder {nullptr};

    static const QStringList folderColumnsName;

    enum {
        folder_id = 0,
        folder_name,
        default_icon,
        icon_path,
        note_count,
        create_time,
        modify_time,
    };
};

#endif // VNOTEFOLDEROPER_H
