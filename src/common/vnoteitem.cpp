/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "vnoteitem.h"

#include <QFile>
#include <QFileInfo>

#include <DLog>

VNoteItem::VNoteItem()
{
}

bool VNoteItem::isValid()
{
    return (noteId > INVALID_ID
            && folderId > INVALID_ID)
               ? true
               : false;
}

void VNoteItem::delNoteData()
{
    //Clear note attachments
    for (auto it : datas.datas) {
        it->releaseSpecificData();
    }
}

bool VNoteItem::search(const QString &keyword)
{
    bool fContainKeyword = false;

    //If title contain keyword,don't
    //need search data anymore.
    if (noteTitle.contains(keyword, Qt::CaseInsensitive)) {
        fContainKeyword = true;
    } else {
        //Need search data blocks in note
        for (auto it : datas.datas) {
            if (VNoteBlock::Text == it->getType()) {
                if (it->blockText.contains(keyword, Qt::CaseInsensitive)) {
                    fContainKeyword = true;
                    break;
                }
            }
        }
    }

    return fContainKeyword;
}

//bool VNoteItem::makeMetaData()
//{
//    bool isMetaDataOk = false;

//    return isMetaDataOk;
//}

void VNoteItem::setMetadata(const QVariant &meta)
{
    metaData = meta;
}

void VNoteItem::setFolder(VNoteFolder *folder)
{
    ownFolder = folder;
}

VNoteFolder *VNoteItem::folder() const
{
    return ownFolder;
}

QVariant &VNoteItem::metaDataRef()
{
    return metaData;
}

const QVariant &VNoteItem::metaDataConstRef() const
{
    return metaData;
}

qint32 &VNoteItem::maxVoiceIdRef()
{
    return maxVoiceId;
}

qint32 VNoteItem::voiceMaxId() const
{
    return maxVoiceId;
}

VNoteBlock *VNoteItem::newBlock(int type)
{
    return datas.newBlock(type);
}

void VNoteItem::addBlock(VNoteBlock *block)
{
    if (VNoteBlock::Voice == block->getType()) {
        maxVoiceId++;
    }

    datas.addBlock(block);
}

void VNoteItem::addBlock(VNoteBlock *before, VNoteBlock *block)
{
    if (VNoteBlock::Voice == block->getType()) {
        maxVoiceId++;
    }

    datas.addBlock(before, block);
}

void VNoteItem::delBlock(VNoteBlock *block)
{
    datas.delBlock(block);
}

bool VNoteItem::haveVoice() const
{
    bool fHaveVoice = false;

    if (datas.voiceBlocks.size() > 0) {
        fHaveVoice = true;
    }

    return fHaveVoice;
}

bool VNoteItem::haveText() const
{
    bool fHaveText = false;

    for (auto it : datas.textBlocks) {
        if (!it->blockText.isEmpty()) {
            fHaveText = true;
            break;
        }
    }

    return fHaveText;
}

qint32 VNoteItem::voiceCount() const
{
    return datas.voiceBlocks.size();
}

QDebug &operator<<(QDebug &out, VNoteItem &noteItem)
{
    out << "\n{ "
        << "noteId=" << noteItem.noteId << ","
        << "folderId=" << noteItem.folderId << ","
        << "noteType=" << noteItem.noteType << ","
        << "noteState=" << noteItem.noteState << ","
        << "noteTitle=" << noteItem.noteTitle << ","
        << "metaData=" << noteItem.metaData << ","
        << "createTime=" << noteItem.createTime << ","
        << "modifyTime=" << noteItem.modifyTime << ","
        << "deleteTime=" << noteItem.deleteTime << ","
        << "maxVoiceId=" << noteItem.maxVoiceId
        << " }\n";

    return out;
}

DetailItemWidget::DetailItemWidget(QWidget *parent)
    : QWidget(parent)
{
    ;
}

void DetailItemWidget::updateSearchKey(QString searchKey)
{
    Q_UNUSED(searchKey);
}

void DetailItemWidget::pasteText()
{
    ;
}

bool DetailItemWidget::isTextContainsPos(const QPoint &globalPos)
{
    Q_UNUSED(globalPos);
    return true;
}

VNoteBlock::VNoteBlock(qint32 type)
    : blockType(type)
{
    ptrBlock = this;
}

VNoteBlock::~VNoteBlock()
{
}

qint32 VNoteBlock::getType()
{
    return blockType;
}

VNTextBlock::VNTextBlock()
    : VNoteBlock(Text)
{
}

VNTextBlock::~VNTextBlock()
{
}

void VNTextBlock::releaseSpecificData()
{
    //TODO:
    //    Add text specific operation code here.
    //
    //Do nothing for text now.
}

VNVoiceBlock::VNVoiceBlock()
    : VNoteBlock(Voice)
{
    blockType = Voice;
}

VNVoiceBlock::~VNVoiceBlock()
{
}

void VNVoiceBlock::releaseSpecificData()
{
    //TODO:
    //    Add voice specific operation code here:
    qInfo() << "Remove file:" << voicePath;

    QFileInfo fileInfo(voicePath);

    if (fileInfo.exists()) {
        QFile::remove(voicePath);
    }
}
