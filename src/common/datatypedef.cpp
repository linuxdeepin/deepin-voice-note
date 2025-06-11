// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatypedef.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "globaldef.h"
#include <QDebug>

// #include <DLog>

/**
 * @brief VNOTE_FOLDERS_MAP::~VNOTE_FOLDERS_MAP
 */
VNOTE_FOLDERS_MAP::~VNOTE_FOLDERS_MAP()
{
    if (autoRelease) {
        qDebug() << "Auto releasing" << folders.size() << "folders";

        for (auto it : folders) {
            delete reinterpret_cast<VNoteFolder *>(it);
        }

        folders.clear();
        qDebug() << "All folders released";
    }
}

/**
 * @brief VNOTE_ITEMS_MAP::~VNOTE_ITEMS_MAP
 */
VNOTE_ITEMS_MAP::~VNOTE_ITEMS_MAP()
{
    if (autoRelease) {
        qDebug() << "Auto releasing" << folderNotes.size() << "notes from folder";

        for (auto it : folderNotes) {
            delete reinterpret_cast<VNoteItem *>(it);
        }

        folderNotes.clear();
        qDebug() << "All folder notes released";
    }
}

/**
 * @brief VNOTE_ALL_NOTES_MAP::~VNOTE_ALL_NOTES_MAP
 */
VNOTE_ALL_NOTES_MAP::~VNOTE_ALL_NOTES_MAP()
{
    if (autoRelease) {
        qDebug() << "Auto releasing all notes from" << notes.size() << "folders";

        for (auto it : notes) {
            delete reinterpret_cast<VNOTE_ITEMS_MAP *>(it);
        }

        notes.clear();
        qDebug() << "All notes released";
    }
}

/**
 * @brief VNOTE_DATAS::~VNOTE_DATAS
 */
VNOTE_DATAS::~VNOTE_DATAS()
{
    qDebug() << "Releasing" << datas.size() << "data blocks";
    for (auto it : datas) {
        delete it;
    }
    qDebug() << "All data blocks released";
}

/**
 * @brief VNOTE_DATAS::dataConstRef
 * @return 记事项数据
 */
const VNOTE_DATA_VECTOR &VNOTE_DATAS::dataConstRef()
{
    return datas;
}

/**
 * @brief VNOTE_DATAS::newBlock
 * @param type
 * @return 新建数据块
 */
VNoteBlock *VNOTE_DATAS::newBlock(int type)
{
    VNoteBlock *ptrBlock = nullptr;

    if (type == VNoteBlock::Text) {
        ptrBlock = new VNTextBlock();
        qDebug() << "Created new text block";
    } else if (type == VNoteBlock::Voice) {
        ptrBlock = new VNVoiceBlock();
        qDebug() << "Created new voice block";
    } else {
        qWarning() << "Attempted to create block with unknown type:" << type;
    }

    return ptrBlock;
}

/**
 * @brief VNOTE_DATAS::addBlock
 * @param block
 */
void VNOTE_DATAS::addBlock(VNoteBlock *block)
{
    datas.push_back(block);

    //Add block to classification data set
    classifyAddBlk(block);
}

/**
 * @brief VNOTE_DATAS::addBlock
 * @param before
 * @param block
 */
void VNOTE_DATAS::addBlock(VNoteBlock *before, VNoteBlock *block)
{
    //If before is null, insert block at the head
    if (nullptr != before) {
        int index = datas.indexOf(before);

        //If the before block is invalid,maybe some errors
        //happened,just add at the end
        if (index != -1) {
            datas.insert(index + 1, block);
        } else {
            datas.append(block);
            qWarning() << "Block not found in datas:" << before 
                      << "appending new block at end:" << block;
        }
    } else {
        datas.insert(0, block);
    }

    //Add block to classification data set
    classifyAddBlk(block);
}

/**
 * @brief VNOTE_DATAS::delBlock
 * @param block
 */
void VNOTE_DATAS::delBlock(VNoteBlock *block)
{
    int index = datas.indexOf(block);

    if (index != -1) {
        datas.remove(index);
    } else {
        qWarning() << "Block not found in datas for deletion:" << block;
    }

    //Remove the block from classification data set
    classifyDelBlk(block);

    delete block;
}

/**
 * @brief VNOTE_DATAS::classifyAddBlk
 * @param block
 */
void VNOTE_DATAS::classifyAddBlk(VNoteBlock *block)
{
    if (VNoteBlock::Text == block->getType()) {
        textBlocks.push_back(block);
        qDebug() << "Added text block to text blocks collection";
    } else if (VNoteBlock::Voice == block->getType()) {
        voiceBlocks.push_back(block);
        qDebug() << "Added voice block to voice blocks collection";
    } else {
        qCritical() << "Unknown VNoteBlock type:" << block->getType();
    }
}

/**
 * @brief VNOTE_DATAS::classifyDelBlk
 * @param block
 */
void VNOTE_DATAS::classifyDelBlk(VNoteBlock *block)
{
    if (VNoteBlock::Text == block->getType()) {
        int index = textBlocks.indexOf(block);

        if (index != -1) {
            textBlocks.remove(index);
            qDebug() << "Removed text block from text blocks collection at index" << index;
        } else {
            qWarning() << "Text block not found in text blocks collection:" << block;
        }
    } else if (VNoteBlock::Voice == block->getType()) {
        int index = voiceBlocks.indexOf(block);

        if (index != -1) {
            voiceBlocks.remove(index);
            qDebug() << "Removed voice block from voice blocks collection at index" << index;
        } else {
            qWarning() << "Voice block not found in voice blocks collection:" << block;
        }
    } else {
        qCritical() << "Unknown VNoteBlock type:" << block->getType();
    }

    //Don't need delete the block anymore,
    //becuase it's already released.
}

/**
 * @brief VDataSafer::isValid
 * @return true可用
 */
bool VDataSafer::isValid() const
{
    //TODO:
    //    Now path field is uique.Other may
    // be empty.So only check it for validation.
    return !(path.isEmpty() || folder_id == INVALID_ID || note_id == INVALID_ID);
}

/**
 * @brief VDataSafer::setSaferType
 * @param type
 */
void VDataSafer::setSaferType(VDataSafer::SaferType type)
{
    saferType = type;
}

QDebug &operator<<(QDebug &out, const VDataSafer &safer)
{
    const QStringList saferTypes = {
        "Safe",
        "UnSafe",
        "ExceptionSafer",
    };

    out << "VDataSafer { "
        << "saferType=" << saferTypes[safer.saferType] << ","
        << "Id=" << safer.id << ","
        << "folder_id=" << safer.folder_id << ","
        << "note_id=" << safer.note_id << ","
        << "path=" << safer.path << ","
        << "state=" << safer.state << ","
        << "meta_data=" << safer.meta_data << ","
        << "createTime=" << safer.createTime.toString(VNOTE_TIME_FMT)
        << " }\n";

    return out;
}
