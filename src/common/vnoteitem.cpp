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

#include <DLog>

#include <QFile>
#include <QFileInfo>

/**
 * @brief VNoteItem::VNoteItem
 */
VNoteItem::VNoteItem()
{
}

/**
 * @brief VNoteItem::isValid
 * @return true 可用
 */
bool VNoteItem::isValid()
{
    return (noteId > INVALID_ID
            && folderId > INVALID_ID)
               ? true
               : false;
}

/**
 * @brief VNoteItem::delNoteData
 */
void VNoteItem::delNoteData()
{
    //Clear note attachments
    for (auto it : datas.datas) {
        it->releaseSpecificData();
    }
}

/**
 * @brief VNoteItem::search
 * @param keyword 搜索关键字
 * @return true 记事项内容包含关键字
 */
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

/**
 * @brief VNoteItem::setMetadata
 * @param meta 源数据
 */
void VNoteItem::setMetadata(const QVariant &meta)
{
    metaData = meta;
}

/**
 * @brief VNoteItem::setFolder
 * @param folder
 */
void VNoteItem::setFolder(VNoteFolder *folder)
{
    ownFolder = folder;
}

/**
 * @brief VNoteItem::folder
 * @return 记事本数据
 */
VNoteFolder *VNoteItem::folder() const
{
    return ownFolder;
}

/**
 * @brief VNoteItem::metaDataRef
 * @return 源数据
 */
QVariant &VNoteItem::metaDataRef()
{
    return metaData;
}

/**
 * @brief VNoteItem::metaDataConstRef
 * @return 源数据
 */
const QVariant &VNoteItem::metaDataConstRef() const
{
    return metaData;
}

/**
 * @brief VNoteItem::maxVoiceIdRef
 * @return 语音项最大id
 */
qint32 &VNoteItem::maxVoiceIdRef()
{
    return maxVoiceId;
}

/**
 * @brief VNoteItem::voiceMaxId
 * @return 语音项最大的id
 */
qint32 VNoteItem::voiceMaxId() const
{
    return maxVoiceId;
}

/**
 * @brief VNoteItem::newBlock
 * @param type
 * @return 生成的数据块
 */
VNoteBlock *VNoteItem::newBlock(int type)
{
    return datas.newBlock(type);
}

/**
 * @brief VNoteItem::addBlock
 * @param block
 */
void VNoteItem::addBlock(VNoteBlock *block)
{
    if (VNoteBlock::Voice == block->getType()) {
        maxVoiceId++;
    }

    datas.addBlock(block);
}

/**
 * @brief VNoteItem::addBlock
 * @param before
 * @param block
 */
void VNoteItem::addBlock(VNoteBlock *before, VNoteBlock *block)
{
    if (VNoteBlock::Voice == block->getType()) {
        maxVoiceId++;
    }

    datas.addBlock(before, block);
}

/**
 * @brief VNoteItem::delBlock
 * @param block
 */
void VNoteItem::delBlock(VNoteBlock *block)
{
    datas.delBlock(block);
}

/**
 * @brief VNoteItem::haveVoice
 * @return true 有语音
 */
bool VNoteItem::haveVoice() const
{
    bool fHaveVoice = false;

    if (datas.voiceBlocks.size() > 0) {
        fHaveVoice = true;
    }

    return fHaveVoice;
}

/**
 * @brief VNoteItem::haveText
 * @return true 有文本
 */
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

/**
 * @brief VNoteItem::voiceCount
 * @return 语音数量
 */
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

/**
 * @brief DetailItemWidget::DetailItemWidget
 * @param parent
 */
DetailItemWidget::DetailItemWidget(QWidget *parent)
    : QWidget(parent)
{
    ;
}

/**
 * @brief DetailItemWidget::updateSearchKey
 * @param searchKey 搜索关键字
 */
void DetailItemWidget::updateSearchKey(QString searchKey)
{
    Q_UNUSED(searchKey);
}

/**
 * @brief DetailItemWidget::pasteText
 */
void DetailItemWidget::pasteText()
{
    ;
}

/**
 * @brief DetailItemWidget::isTextContainsPos
 * @param globalPos 全局坐标点
 * @return true 坐标点在文本区域内
 */
bool DetailItemWidget::isTextContainsPos(const QPoint &globalPos)
{
    Q_UNUSED(globalPos);
    return true;
}

/**
 * @brief VNoteBlock::VNoteBlock
 * @param type
 */
VNoteBlock::VNoteBlock(qint32 type)
    : blockType(type)
{
    ptrBlock = this;
}

/**
 * @brief VNoteBlock::~VNoteBlock
 */
VNoteBlock::~VNoteBlock()
{
}

/**
 * @brief VNoteBlock::getType
 * @return 数据类型
 */
qint32 VNoteBlock::getType()
{
    return blockType;
}

/**
 * @brief VNTextBlock::VNTextBlock
 */
VNTextBlock::VNTextBlock()
    : VNoteBlock(Text)
{
}

VNTextBlock::~VNTextBlock()
{
}

/**
 * @brief VNTextBlock::releaseSpecificData
 */
void VNTextBlock::releaseSpecificData()
{
    //TODO:
    //    Add text specific operation code here.
    //
    //Do nothing for text now.
}

/**
 * @brief VNVoiceBlock::VNVoiceBlock
 */
VNVoiceBlock::VNVoiceBlock()
    : VNoteBlock(Voice)
{
    blockType = Voice;
}

VNVoiceBlock::~VNVoiceBlock()
{
}

/**
 * @brief VNVoiceBlock::releaseSpecificData
 */
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
