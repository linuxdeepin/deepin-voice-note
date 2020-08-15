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

#include <DLog>

/**
 * @brief MetaDataParser::MetaDataParser
 */
MetaDataParser::MetaDataParser()
{
}

/**
 * @brief MetaDataParser::parse
 * @param metaData 数据源
 * @param noteData 解析后的数据
 */
void MetaDataParser::parse(const QVariant &metaData, VNoteItem *noteData)
{
#ifdef VN_XML_METADATA_PARSER
    xmlParse(metaData, noteData);
#elif defined(VN_JSON_METADATA_PARSER)
    jsonParse(metaData, noteData);
#endif
}

/**
 * @brief MetaDataParser::makeMetaData
 * @param noteData 数据源
 * @param metaData 生成的数据
 */
void MetaDataParser::makeMetaData(const VNoteItem *noteData, QVariant &metaData)
{
#ifdef VN_XML_METADATA_PARSER
    xmlMakeMetadata(noteData, metaData);
#elif defined(VN_JSON_METADATA_PARSER)
    jsonMakeMetadata(noteData, metaData);
#endif
}

//*****************Implementation of xml meta-data parser***********************
#ifdef VN_XML_METADATA_PARSER
void MetaDataParser::xmlParse(const QVariant &metaData, VNoteItem *noteData)
{
    Q_ASSERT(nullptr != noteData);

    QXmlStreamReader xmlStreamReader(metaData.toString());

    while (xmlStreamReader.readNextStartElement()) {
        if (m_xmlNodeNameMap[NRootNode] == xmlStreamReader.name()) {
            xmlParseRoot(xmlStreamReader, noteData);

        } else if (m_xmlNodeNameMap[NItemNode] == xmlStreamReader.name()) {
            xmlParseNoteItem(xmlStreamReader, noteData);
        } else {
            xmlStreamReader.skipCurrentElement();
        }
    }
}

void MetaDataParser::xmlMakeMetadata(const VNoteItem *noteData, QVariant &metaData)
{
    Q_ASSERT(nullptr != noteData);

    QString strMetaData = metaData.toString();
    QXmlStreamWriter xmlStreamWriter(&strMetaData);

    xmlStreamWriter.writeStartDocument();
    xmlStreamWriter.writeStartElement(m_xmlNodeNameMap[NRootNode]);
    xmlStreamWriter.writeAttribute(m_xmlNodeNameMap[NItemCountAttr], QString("%1").arg(noteData->datas.datas.length()));
    xmlStreamWriter.writeAttribute(m_xmlNodeNameMap[NVoiceMaxIdAttr], QString("%1").arg(noteData->voiceMaxId()));

    if (noteData->datas.datas.length() > 0) {
        for (auto it : noteData->datas.datas) {
            if (VNoteBlock::Text == it->getType()) {
                xmlStreamWriter.writeStartElement(m_xmlNodeNameMap[NItemNode]);
                xmlStreamWriter.writeAttribute(m_xmlNodeNameMap[NItemTypeAttr],
                                               QString("%1").arg(VNoteBlock::Text));

                xmlStreamWriter.writeTextElement(m_xmlNodeNameMap[NTextNode],
                                                 it->ptrText->blockText);
                xmlStreamWriter.writeEndElement();
            } else if (VNoteBlock::Voice == it->getType()) {
                xmlStreamWriter.writeStartElement(m_xmlNodeNameMap[NItemNode]);
                xmlStreamWriter.writeAttribute(m_xmlNodeNameMap[NItemTypeAttr],
                                               QString("%1").arg(VNoteBlock::Voice));

                xmlStreamWriter.writeTextElement(m_xmlNodeNameMap[NVoicePathNode],
                                                 it->ptrVoice->voicePath);
                xmlStreamWriter.writeTextElement(m_xmlNodeNameMap[NVoiceSizeNode],
                                                 QString("%1").arg(it->ptrVoice->voiceSize));
                xmlStreamWriter.writeTextElement(m_xmlNodeNameMap[NVoiceTitleNode],
                                                 it->ptrVoice->voiceTitle);
                xmlStreamWriter.writeTextElement(m_xmlNodeNameMap[NVoiceTextNode],
                                                 it->ptrVoice->blockText);
                xmlStreamWriter.writeTextElement(m_xmlNodeNameMap[NVoiceStateNode],
                                                 QString("%1").arg(it->ptrVoice->state));
                xmlStreamWriter.writeTextElement(m_xmlNodeNameMap[NVoiceCreateTimeNode],
                                                 it->ptrVoice->createTime.toString(VNOTE_TIME_FMT));

                xmlStreamWriter.writeEndElement();
            } else {
                //Must never reach here
                qInfo() << "Invalid VoiceBlock:" << it->getType();
            }
        }
    }

    xmlStreamWriter.writeEndElement();
    xmlStreamWriter.writeEndDocument();

    metaData = strMetaData;
}

void MetaDataParser::xmlParseRoot(QXmlStreamReader &xmlSRead, VNoteItem *noteData)
{
    QXmlStreamAttributes noteItemCountAttr = xmlSRead.attributes();

    if (noteItemCountAttr.hasAttribute(m_xmlNodeNameMap[NItemCountAttr])) {
        noteItemCountAttr.value(m_xmlNodeNameMap[NItemCountAttr]).toInt();
        //        qInfo() << "noteItemCountAttr:" << count;
    }

    if (noteItemCountAttr.hasAttribute(m_xmlNodeNameMap[NVoiceMaxIdAttr])) {
        noteData->maxVoiceIdRef() = noteItemCountAttr.value(m_xmlNodeNameMap[NVoiceMaxIdAttr]).toInt();
    }
}

void MetaDataParser::xmlParseNoteItem(QXmlStreamReader &xmlSRead, VNoteItem *noteData)
{
    QXmlStreamAttributes noteItemTypeAttr = xmlSRead.attributes();

    int noteItemType = VNoteBlock::InValid;

    if (noteItemTypeAttr.hasAttribute(m_xmlNodeNameMap[NItemTypeAttr])) {
        noteItemType = noteItemTypeAttr.value(m_xmlNodeNameMap[NItemTypeAttr]).toInt();
    }

    if (noteItemType != VNoteBlock::InValid) {
        VNoteBlock *ptrBlock = nullptr;

        //Allocate block
        ptrBlock = noteData->datas.newBlock(noteItemType);

        while (xmlSRead.readNextStartElement()) {
            if (VNoteBlock::Text == noteItemType) {
                if (xmlSRead.name() == m_xmlNodeNameMap[NTextNode]) {
                    ptrBlock->ptrText->blockText = xmlSRead.readElementText();
                }

            } else if (VNoteBlock::Voice == noteItemType) {
                if (xmlSRead.name() == m_xmlNodeNameMap[NVoicePathNode]) {
                    ptrBlock->ptrVoice->voicePath = xmlSRead.readElementText();
                } else if (xmlSRead.name() == m_xmlNodeNameMap[NVoiceSizeNode]) {
                    ptrBlock->ptrVoice->voiceSize = QString(xmlSRead.readElementText()).toLong();
                } else if (xmlSRead.name() == m_xmlNodeNameMap[NVoiceTitleNode]) {
                    ptrBlock->ptrVoice->voiceTitle = xmlSRead.readElementText();
                } else if (xmlSRead.name() == m_xmlNodeNameMap[NVoiceTextNode]) {
                    ptrBlock->ptrVoice->blockText = xmlSRead.readElementText();
                } else if (xmlSRead.name() == m_xmlNodeNameMap[NVoiceStateNode]) {
                    ptrBlock->ptrVoice->state = QString("%1").arg(xmlSRead.readElementText()).toInt();
                } else if (xmlSRead.name() == m_xmlNodeNameMap[NVoiceCreateTimeNode]) {
                    ptrBlock->ptrVoice->createTime = QDateTime::fromString(xmlSRead.readElementText(), VNOTE_TIME_FMT);
                }
            } else {
                xmlSRead.skipCurrentElement();
            }
        } // End while

        //Parse note item end
        noteData->datas.addBlock(ptrBlock);
    }
}

#elif defined(VN_JSON_METADATA_PARSER)
//*****************Implementation of json meta-data parser**********************

/**
 * @brief MetaDataParser::jsonParse
 * @param metaData 数据源
 * @param noteData 解析的数据
 */
void MetaDataParser::jsonParse(const QVariant &metaData, VNoteItem *noteData /*out*/)
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

/**
 * @brief MetaDataParser::jsonMakeMetadata
 * @param noteData 数据源
 * @param metaData 生成的数据
 */
void MetaDataParser::jsonMakeMetadata(const VNoteItem *noteData, QVariant &metaData)
{
    Q_ASSERT(nullptr != noteData);

    QJsonDocument noteDoc;

    QJsonObject note;
    QJsonArray noteDatas;
    int noteCount = noteData->datas.datas.length();

    for (auto it : noteData->datas.datas) {
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
            }

            noteDatas.append(noteItem);
        }
    }

    note.insert(m_jsonNodeNameMap[NDataCount], noteCount);
    note.insert(m_jsonNodeNameMap[NVoiceMaxId], noteData->voiceMaxId());
    note.insert(m_jsonNodeNameMap[NDatas], noteDatas);

    noteDoc.setObject(note);

    //qDebug() << noteDoc.toJson();

    metaData = noteDoc.toJson(QJsonDocument::Compact);
}

#endif
