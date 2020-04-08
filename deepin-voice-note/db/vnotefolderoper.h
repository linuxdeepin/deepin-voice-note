#ifndef VNOTEFOLDEROPER_H
#define VNOTEFOLDEROPER_H

#include "common/datatypedef.h"

#include <QPixmap>

class DbVisitor;
class VNoteItemOper;

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

    static const QStringList folderColumnsName;

    enum {
        folder_id = 0,
        category_id,
        folder_name,
        default_icon,
        icon_path,
        folder_state,
        max_noteid,
        create_time,
        modify_time,
        delete_time,
    };

    friend class FolderQryDbVisitor;
    friend class AddFolderDbVisitor;
    friend class VNoteItemOper;  //Need folder feilds
};

#endif // VNOTEFOLDEROPER_H
