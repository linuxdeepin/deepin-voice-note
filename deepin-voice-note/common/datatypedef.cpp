#include "datatypedef.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"

VNOTE_FOLDERS_MAP::~VNOTE_FOLDERS_MAP()
{
    for (auto it : folders) {
        delete  reinterpret_cast<VNoteFolder*>(it);
    }

    folders.clear();
}

VNOTE_ITEMS_MAP::~VNOTE_ITEMS_MAP()
{
    for (auto it : folderNotes) {
        delete  reinterpret_cast<VNoteItem*>(it);
    }

    folderNotes.clear();
}

VNOTE_ALL_NOTES_MAP::~VNOTE_ALL_NOTES_MAP()
{
    for (auto it : notes) {
        delete  reinterpret_cast<VNOTE_ITEMS_MAP*>(it);
    }

    notes.clear();
}
