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
#include "metadataparser.h"
#include "vnoteitem.h"
#include "common/utils.h"

#include <DLog>

/**
 * @brief MetaDataParser::MetaDataParser
 */
MetaDataParser::MetaDataParser()
{
}

void MetaDataParser::jsonParse(const QVariant &metaData, VNoteBlock *blockData)
{
    QJsonDocument noteDoc = QJsonDocument::fromJson(metaData.toByteArray());
    QJsonObject note = noteDoc.object();
    int noteType = note.value(m_jsonNodeNameMap[NDataType]).toInt(VNoteBlock::InValid);
    if (VNoteBlock::InValid != noteType) {

        blockData->blockType = noteType;

        if (VNoteBlock::Text == noteType) {
            blockData->ptrText->blockText = note.value(m_jsonNodeNameMap[NText]).toString();
        } else if (VNoteBlock::Voice == noteType) {
            blockData->ptrVoice->blockText = note.value(m_jsonNodeNameMap[NText]).toString();
            blockData->ptrVoice->voiceTitle = note.value(m_jsonNodeNameMap[NTitle]).toString();
            blockData->ptrVoice->state = note.value(m_jsonNodeNameMap[NState]).toBool(false);
            blockData->ptrVoice->voicePath = note.value(m_jsonNodeNameMap[NVoicePath]).toString();
            blockData->ptrVoice->voiceSize = note.value(m_jsonNodeNameMap[NVoiceSize]).toInt(0);
            blockData->ptrVoice->createTime = QDateTime::fromString(
                        note.value(m_jsonNodeNameMap[NCreateTime]).toString(), VNOTE_TIME_FMT);
        }
    }
}

void MetaDataParser::jsonParse(QVariant &metaData, QVector<VNoteBlock *> &blocksData)
{

    QJsonDocument noteDoc = QJsonDocument::fromJson(metaData.toByteArray());
    QJsonObject note = noteDoc.object();
    QJsonArray noteDatas;
    if (note.contains(m_jsonNodeNameMap[NDatas])) {
        noteDatas = note.value(m_jsonNodeNameMap[NDatas]).toArray();
    }
    for (auto it : noteDatas) {
        VNoteBlock *ptrBlock = nullptr;

        QJsonObject noteItem = it.toObject();

        int noteType = noteItem.value(m_jsonNodeNameMap[NDataType]).toInt(VNoteBlock::InValid);

        if (VNoteBlock::InValid != noteType) {
            //Allocate block

            if (VNoteBlock::Text == noteType) {
                ptrBlock = new VNTextBlock;
                ptrBlock->blockType = noteType;
                ptrBlock->ptrText->blockText = noteItem.value(m_jsonNodeNameMap[NText]).toString();
            } else if (VNoteBlock::Voice == noteType) {
                ptrBlock = new VNVoiceBlock;
                ptrBlock->ptrVoice->blockText = noteItem.value(m_jsonNodeNameMap[NText]).toString();
                ptrBlock->ptrVoice->voiceTitle = noteItem.value(m_jsonNodeNameMap[NTitle]).toString();
                ptrBlock->ptrVoice->state = noteItem.value(m_jsonNodeNameMap[NState]).toBool(false);
                ptrBlock->ptrVoice->voicePath = noteItem.value(m_jsonNodeNameMap[NVoicePath]).toString();
                ptrBlock->ptrVoice->voiceSize = noteItem.value(m_jsonNodeNameMap[NVoiceSize]).toInt(0);
                ptrBlock->ptrVoice->createTime = QDateTime::fromString(
                            noteItem.value(m_jsonNodeNameMap[NCreateTime]).toString(), VNOTE_TIME_FMT);
            }
            if(ptrBlock){
                blocksData.push_back(ptrBlock);
            }
        }
    }

}

/**
 * @brief MetaDataParser::jsonParse
 * @param metaData 数据源
 * @param noteData 解析的数据
 */
void MetaDataParser::jsonParse(QVariant &metaData, VNoteItem *noteData /*out*/)
{
    Q_ASSERT(nullptr != noteData);

    QJsonDocument noteDoc = QJsonDocument::fromJson(metaData.toByteArray());

    QJsonObject note = noteDoc.object();
    QJsonArray noteDatas;
    //    int         noteCount = -1;

    //    //noteCount doesn't used now
    //    if (note.contains(m_jsonNodeNameMap[NDataCount])) {
    //        noteCount = note.value(m_jsonNodeNameMap[NDataCount]).toInt(0);
    //    }

    if(note.contains(m_jsonNodeNameMap[NHtmlCode])){
        noteData->htmlCode = note.value(m_jsonNodeNameMap[NHtmlCode]).toString();
        return;
    }

    //Get default voice max id
    if (note.contains(m_jsonNodeNameMap[NVoiceMaxId])) {
        noteData->maxVoiceIdRef() = note.value(m_jsonNodeNameMap[NVoiceMaxId]).toInt(0);
    }

    if (note.contains(m_jsonNodeNameMap[NDatas])) {
        noteDatas = note.value(m_jsonNodeNameMap[NDatas]).toArray();
    }

    //Parse the note datas
    for (auto it : noteDatas) {
        VNoteBlock *ptrBlock = nullptr;

        QJsonObject noteItem = it.toObject();

        int noteType = noteItem.value(m_jsonNodeNameMap[NDataType]).toInt(VNoteBlock::InValid);

        if (VNoteBlock::InValid != noteType) {
            //Allocate block
            ptrBlock = noteData->datas.newBlock(noteType);

            if (VNoteBlock::Text == noteType) {
                ptrBlock->ptrText->blockText = noteItem.value(m_jsonNodeNameMap[NText]).toString();
            } else if (VNoteBlock::Voice == noteType) {
                ptrBlock->ptrVoice->blockText = noteItem.value(m_jsonNodeNameMap[NText]).toString();
                ptrBlock->ptrVoice->voiceTitle = noteItem.value(m_jsonNodeNameMap[NTitle]).toString();
                ptrBlock->ptrVoice->state = noteItem.value(m_jsonNodeNameMap[NState]).toBool(false);
                ptrBlock->ptrVoice->voicePath = noteItem.value(m_jsonNodeNameMap[NVoicePath]).toString();
                ptrBlock->ptrVoice->voiceSize = noteItem.value(m_jsonNodeNameMap[NVoiceSize]).toInt(0);
                ptrBlock->ptrVoice->createTime = QDateTime::fromString(
                            noteItem.value(m_jsonNodeNameMap[NCreateTime]).toString(), VNOTE_TIME_FMT);
            }

            noteData->datas.addBlock(ptrBlock);
        }
    }
}

void MetaDataParser::jsonMakeMetadata(VNoteBlock *blockData, QVariant &metaData)
{
    QJsonDocument noteDoc;
    QJsonObject note;
    int noteType = blockData->getType();
    if (VNoteBlock::InValid != noteType) {
        note.insert(m_jsonNodeNameMap[NDataType], blockData->getType());
        if (VNoteBlock::Text == noteType) {
            note.insert(m_jsonNodeNameMap[NText], blockData->ptrText->blockText);
        } else if (VNoteBlock::Voice == noteType) {
            note.insert(m_jsonNodeNameMap[NText], blockData->ptrVoice->blockText);
            note.insert(m_jsonNodeNameMap[NTitle], blockData->ptrVoice->voiceTitle);
            note.insert(m_jsonNodeNameMap[NState], blockData->ptrVoice->state);
            note.insert(m_jsonNodeNameMap[NVoicePath], blockData->ptrVoice->voicePath);
            note.insert(m_jsonNodeNameMap[NVoiceSize], blockData->ptrVoice->voiceSize);
            note.insert(m_jsonNodeNameMap[NCreateTime],
                            blockData->ptrVoice->createTime.toString(VNOTE_TIME_FMT));
            note.insert(m_jsonNodeNameMap[NFormatSize], Utils::formatMillisecond(blockData->ptrVoice->voiceSize));
        }
    }
    noteDoc.setObject(note);
    metaData = noteDoc.toJson(QJsonDocument::Compact);
}

/**
 * @brief MetaDataParser::jsonMakeMetadata
 * @param noteData 数据源
 * @param metaData 生成的数据
 */
void MetaDataParser::jsonMakeMetadata(VNoteItem *noteData, QVariant &metaData)
{
    Q_ASSERT(nullptr != noteData);

    QJsonDocument noteDoc;

    QJsonObject note;
    QJsonArray noteDatas;
    int noteCount = 0;
    bool htmlIsEmpty = noteData->htmlCode.isEmpty();
    QList<VNoteBlock*> delBlocks;
    for (auto it : noteData->datas.datas) {
        if(!htmlIsEmpty){
            delBlocks.push_back(it);
            continue;
        }
        int noteType = it->getType();
        if (VNoteBlock::InValid != noteType) {
            QJsonObject noteItem;
            noteItem.insert(m_jsonNodeNameMap[NDataType], it->getType());
            if (VNoteBlock::Text == noteType) {
                noteItem.insert(m_jsonNodeNameMap[NText], it->ptrText->blockText);
            } else if (VNoteBlock::Voice == noteType) {
                noteItem.insert(m_jsonNodeNameMap[NText], it->ptrVoice->blockText);
                noteItem.insert(m_jsonNodeNameMap[NTitle], it->ptrVoice->voiceTitle);
                noteItem.insert(m_jsonNodeNameMap[NState], it->ptrVoice->state);
                noteItem.insert(m_jsonNodeNameMap[NVoicePath], it->ptrVoice->voicePath);
                noteItem.insert(m_jsonNodeNameMap[NVoiceSize], it->ptrVoice->voiceSize);
                noteItem.insert(m_jsonNodeNameMap[NCreateTime],
                                it->ptrVoice->createTime.toString(VNOTE_TIME_FMT));
                noteItem.insert(m_jsonNodeNameMap[NFormatSize], Utils::formatMillisecond(it->ptrVoice->voiceSize));
            }
            noteDatas.append(noteItem);
            noteCount++;
        }
    }

    if(!htmlIsEmpty){
        note.insert(m_jsonNodeNameMap[NHtmlCode], noteData->htmlCode);
        for (auto it : delBlocks){
            noteData->delBlock(it);
        }
    }else {
        note.insert(m_jsonNodeNameMap[NDataCount], noteCount);
        note.insert(m_jsonNodeNameMap[NVoiceMaxId], noteData->voiceMaxId());
        note.insert(m_jsonNodeNameMap[NDatas], noteDatas);
    }

    noteDoc.setObject(note);

    //qDebug() << noteDoc.toJson();

    metaData = noteDoc.toJson(QJsonDocument::Compact);
}
