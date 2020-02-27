#ifndef METADATAPARSER_H
#define METADATAPARSER_H

#include "common/datatypedef.h"

#include <QString>
#include <QMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

/*
Eg: meta-data format

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
 */
class MetaDataParser
{
public:
    MetaDataParser();

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

    void parse(const QString &metaData, VNOTE_DATAS &datas/*out*/);
    void makeMetaData(const VNOTE_DATAS &datas,QString &metaData /*out*/);

protected:
    const QMap<int,QString> m_nodeNameMap = {
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

    void parseRoot(QXmlStreamReader& xmlSRead, int& count);
    void parseNoteItem(QXmlStreamReader& xmlSRead, VNOTE_DATAS &datas/*out*/);
};

#endif // METADATAPARSER_H
