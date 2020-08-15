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
#ifndef METADATAPARSER_H
#define METADATAPARSER_H

#include "globaldef.h"
#include "common/datatypedef.h"

#include <QString>
#include <QMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

/*
Eg: meta-data format

    xml-format:

    <?xml version="1.0" encoding="UTF-8"?>
    <Note NoteElementsCount="3" VoiceMaxId="1">
        <NoteItem NoteItemType="1">
            <NoteText>I Love China</NoteText>
        </NoteItem>
        <NoteItem NoteItemType="2">;
            <NoteVoicePath>/home/archermind/1.mp3</NoteVoicePath>
            <NoteVoiceSize>1134556</NoteVoiceSize>
            <NoteVoiceTitle>语音1</NoteVoiceTitle>
            <NoteVoiceText></NoteVoiceText>
            <NoteVoiceState>0</NoteVoiceState>
            <NoteVoiceCreateTime>2020-02-27 10:21:55.528</NoteVoiceCreateTime>
        </NoteItem>
    </Note>

    //------------------------------------------------------------------------
    json-format:

    {
        "dataCount": 2,
        "voiceMaxId":1,
        "noteDatas": [
            {
                "text": "I Love China.",
                "type": 1
            },
            {
                "createTime": "2020-02-27 17:56:36.071",
                "state": false,
                "text": "",
                "title": "语音2",
                "type": 2,
                "voicePath": "/home/voice/1.mp3",
                "voiceSize": 15555
            }
        ]
    }
 */

class MetaDataParser
{
public:
    MetaDataParser();

#ifdef VN_XML_METADATA_PARSER
    enum {
        NRootNode,
        NVoiceMaxIdAttr,
        NItemCountAttr,
        NItemNode,
        NItemTypeAttr,
        NTextNode,
        NVoicePathNode,
        NVoiceSizeNode,
        NVoiceTitleNode,
        NVoiceCreateTimeNode,
        NVoiceStateNode, // State: 1 for audio to text done, 0 for have not done
        NVoiceTextNode,
    };
#elif defined(VN_JSON_METADATA_PARSER)
    enum {
        NDataCount,
        NVoiceMaxId,
        NDatas,
        NDataType,
        NText,
        NTitle,
        NState,
        NVoicePath,
        NVoiceSize,
        NCreateTime,
    };
#endif
    //源数据解析
    void parse(const QVariant &metaData, VNoteItem *noteData /*out*/);
    //源数据生成
    void makeMetaData(const VNoteItem *noteData, QVariant &metaData /*out*/);

protected:
#ifdef VN_XML_METADATA_PARSER
    const QMap<int, QString> m_xmlNodeNameMap = {
        {NRootNode, "Note"},
        {NVoiceMaxIdAttr, "VoiceMaxId"},
        {NItemCountAttr, "NoteElementsCount"},
        {NItemNode, "NoteItem"},
        {NItemTypeAttr, "NoteItemType"},
        {NTextNode, "NoteText"},
        {NVoicePathNode, "NoteVoicePath"},
        {NVoiceSizeNode, "NoteVoiceSize"},
        {NVoiceTitleNode, "NoteVoiceTitle"},
        {NVoiceCreateTimeNode, "NoteVoiceCreateTime"},
        {NVoiceStateNode, "NoteVoiceState"},
        {NVoiceTextNode, "NoteVoiceText"},
    };

    void xmlParse(const QVariant &metaData, VNoteItem *noteData /*out*/);
    void xmlMakeMetadata(const VNoteItem *noteData, QVariant &metaData /*out*/);
    void xmlParseRoot(QXmlStreamReader &xmlSRead, VNoteItem *noteData);
    void xmlParseNoteItem(QXmlStreamReader &xmlSRead, VNoteItem *noteData /*out*/);

#elif defined(VN_JSON_METADATA_PARSER)
    QMap<int, QString> m_jsonNodeNameMap = {
        {NDataCount, "dataCount"},
        {NVoiceMaxId, "voiceMaxId"},
        {NDatas, "noteDatas"},
        {NDataType, "type"},
        {NText, "text"},
        {NTitle, "title"},
        {NState, "state"}, // State: 1 for audio to text done, 0 for have not done
        {NVoicePath, "voicePath"},
        {NVoiceSize, "voiceSize"},
        {NCreateTime, "createTime"},
    };
    //json串解析
    void jsonParse(const QVariant &metaData, VNoteItem *noteData /*out*/);
    //json串生成
    void jsonMakeMetadata(const VNoteItem *noteData, QVariant &metaData /*out*/);
#endif
};

#endif // METADATAPARSER_H
