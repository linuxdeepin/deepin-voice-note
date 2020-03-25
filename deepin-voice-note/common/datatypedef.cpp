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

const VNOTE_DATA_VECTOR &VNOTE_DATAS::dataConstRef()
{
    return datas;
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

    //Add block to classification data set
    classifyAddBlk(block);
}

void VNOTE_DATAS::addBlock(VNoteBlock *before, VNoteBlock *block)
{
    //If before is null, insert block at the head
    if (nullptr != before) {
        int index = datas.indexOf(before);

        //If the before block is invalid,maybe some errors
        //happened,just add at the end
        if (index != -1) {
            datas.insert(index+1, block);
        } else {
            datas.append(block);
            qInfo() << "Block not in datas:" << before
                    << " append at end:" << block;
        }
    } else {
        datas.insert(0, block);
    }

    //Add block to classification data set
    classifyAddBlk(block);
}

void VNOTE_DATAS::delBlock(VNoteBlock *block)
{
    int index = datas.indexOf(block);

    if (index != -1) {
        datas.remove(index);
    }

    //Remove the block from classification data set
    classifyDelBlk(block);

    delete block;
}

void VNOTE_DATAS::classifyAddBlk(VNoteBlock *block)
{
    if (VNoteBlock::Text == block->getType()) {
        textBlocks.push_back(block);
    } else if (VNoteBlock::Voice == block->getType()) {
        voiceBlocks.push_back(block);
    } else {
        qCritical() << __FUNCTION__ << "Unkown VNoteBlock type " << block->getType();
    }
}

void VNOTE_DATAS::classifyDelBlk(VNoteBlock *block)
{
    if (VNoteBlock::Text == block->getType()) {
        int index = textBlocks.indexOf(block);

        if (textBlocks.indexOf(block) != -1) {
            textBlocks.remove(index);
        }
    } else if (VNoteBlock::Voice == block->getType()) {
        int index = voiceBlocks.indexOf(block);

        if (voiceBlocks.indexOf(block) != -1) {
            voiceBlocks.remove(index);
        }
    } else {
        qCritical() << __FUNCTION__ << "Unkown VNoteBlock type " << block->getType();
    }

    //Don't need delete the block anymore,
    //becuase it's already released.
}
