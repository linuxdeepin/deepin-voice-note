#include "datatypedef.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"

#include <DLog>

VNOTE_FOLDERS_MAP::~VNOTE_FOLDERS_MAP()
{
    if (autoRelease) {
        qInfo() << __FUNCTION__ << "Auto release folders";

        for (auto it : folders) {
            delete  reinterpret_cast<VNoteFolder*>(it);
        }

        folders.clear();
    }
}

VNOTE_ITEMS_MAP::~VNOTE_ITEMS_MAP()
{
    if (autoRelease) {
        qInfo() << __FUNCTION__ << "Auto release folder notes";

        for (auto it : folderNotes) {
            delete  reinterpret_cast<VNoteItem*>(it);
        }

        folderNotes.clear();
    }
}

VNOTE_ALL_NOTES_MAP::~VNOTE_ALL_NOTES_MAP()
{
    if (autoRelease) {
        qInfo() << __FUNCTION__ << "Auto release all notes in folders ";

        for (auto it : notes) {
            delete  reinterpret_cast<VNOTE_ITEMS_MAP*>(it);
        }

        notes.clear();
    }
}
