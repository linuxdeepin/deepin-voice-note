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

VNOTE_DATAS::~VNOTE_DATAS()
{
    for (auto it : datas) {
        delete it;
    }
}

VNoteBlock* VNOTE_DATAS::newBlock(int type)
{
    VNoteBlock* ptrBlock = nullptr;

    if (type == VNoteBlock::Text) {
        ptrBlock = new VNTextBlock();
    } else if (type == VNoteBlock::Voice) {
        ptrBlock = new VNVoiceBlock();
    }

    return ptrBlock;
}

void VNOTE_DATAS::addBlock(VNoteBlock *block)
{
    datas.push_back(block);
}

void VNOTE_DATAS::delBlock(VNoteBlock *block)
{
    int index = datas.indexOf(block);

    if (index != -1) {
        datas.remove(index);
    }

    delete block;
}
