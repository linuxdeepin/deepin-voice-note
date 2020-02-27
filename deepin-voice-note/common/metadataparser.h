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
    <Note NoteElementsCount="3">
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
        NItemCountAttr,
        NItemNode,
        NItemTypeAttr,
        NTextNode,
        NVoicePathNode,
        NVoiceSizeNode,
        NVoiceTitleNode,
        NVoiceCreateTimeNode,
        NVoiceStateNode,      // State: 1 for audio to text done, 0 for have not done
        NVoiceTextNode,
    };
#elif defined (VN_JSON_METADATA_PARSER)
    enum {
        NDataCount,
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

    void parse(const QVariant &metaData, VNOTE_DATAS &datas/*out*/);
    void makeMetaData(const VNOTE_DATAS &datas, QVariant &metaData /*out*/);

protected:
#ifdef VN_XML_METADATA_PARSER
    const QMap<int,QString> m_xmlNodeNameMap = {
        {NRootNode, "Note"},
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

    void xmlParse(const QVariant &metaData, VNOTE_DATAS &datas/*out*/);
    void xmlMakeMetadata(const VNOTE_DATAS &datas, QVariant &metaData /*out*/);
    void xmlParseRoot(QXmlStreamReader& xmlSRead, int& count);
    void xmlParseNoteItem(QXmlStreamReader& xmlSRead, VNOTE_DATAS &datas/*out*/);

#elif defined (VN_JSON_METADATA_PARSER)
    QMap<int,QString> m_jsonNodeNameMap = {
            {NDataCount, "dataCount"},
            {NDatas, "noteDatas"},
            {NDataType, "type"},
            {NText, "text"},
            {NTitle, "title"},
            {NState, "state"},          // State: 1 for audio to text done, 0 for have not done
            {NVoicePath, "voicePath"},
            {NVoiceSize, "voiceSize"},
            {NCreateTime, "createTime"},
        };

    void jsonParse(const QVariant &metaData, VNOTE_DATAS &datas/*out*/);
    void jsonMakeMetadata(const VNOTE_DATAS &datas, QVariant &metaData /*out*/);
#endif
};

#endif // METADATAPARSER_H
