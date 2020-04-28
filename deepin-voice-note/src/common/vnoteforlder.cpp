#include "vnoteforlder.h"
#include "common/vnotedatamanager.h"

#include <DLog>

VNoteFolder::VNoteFolder()
{
}

VNoteFolder::~VNoteFolder()
{
}

bool VNoteFolder::isValid()
{
    return (id > INVALID_ID) ? true : false;
}

qint32 &VNoteFolder::maxNoteIdRef()
{
    return maxNoteId;
}

qint32 VNoteFolder::getNotesCount()
{
    int nCount =  0;

    VNOTE_ITEMS_MAP * folderNotes = getNotes();

    if (Q_LIKELY(nullptr != folderNotes)) {
        folderNotes->lock.lockForRead();
        nCount = folderNotes->folderNotes.count();
        folderNotes->lock.unlock();
    }

    return nCount;
}

VNOTE_ITEMS_MAP * VNoteFolder::getNotes()
{
    if (Q_UNLIKELY(nullptr == notes)) {
        VNOTE_ITEMS_MAP * folderNotes =VNoteDataManager::instance()->getFolderNotes(id);
        notes = folderNotes;
    }

    return notes;
}

QDebug & operator <<(QDebug &out, VNoteFolder &folder)
{
    out << "\n{ "
        << "id=" << folder.id << ","
        << "name=" << folder.name << ","
        << "defaultIcon=" << folder.defaultIcon << ","
        << "iconPath=" << folder.iconPath << ","
        << "notesCount=" << folder.notesCount << ","
        << "folder_state=" << folder.folder_state << ","
        << "maxNoteId=" << folder.maxNoteId << ","
        << "createTime=" << folder.createTime << ","
        << "modifyTime=" << folder.modifyTime << ","
        << "deleteTime=" << folder.deleteTime
        << " }\n";

    return out;
}
