#ifndef METADATAPARSER_H
#define METADATAPARSER_H

#include "common/datatypedef.h"

#include <QString>
#include <QMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

/*
Eg: xml format

    <?xml version="1.0" encoding="UTF-8"?>
    <Note NoteElementsCount="3">
        <NoteItem NoteItemType="1">
            <NoteText>I Love China</NoteText>
        </NoteItem>
        <NoteItem NoteItemType="2">;
            <NoteVoicePath>/home/archermind/1.mp3</NoteVoicePath>
            <NoteVoiceSize>135555</NoteVoiceSize>
        </NoteItem>
        <NoteItem NoteItemType="2">
            <NoteVoicePath>/home/archermind/2.mp3</NoteVoicePath>
            <NoteVoiceSize>135555</NoteVoiceSize>
        </NoteItem>
    </Note>
 */
class MetaDataParser
{
public:
    MetaDataParser();

    enum {
        NoteRootNode,
        NoteItemCountAttr,
        NoteItemNode,
        NoteItemTypeAttr,
        NoteTextNode,
        NoteVoicePathNode,
        NoteVoiceSizeNode,
    };

    void parse(const QString &metaData, VNoteDatas &datas/*out*/);
    void makeMetaData(const VNoteDatas &datas,QString &metaData /*out*/);

protected:
    const QMap<int,QString> m_nodeNameMap = {
         {NoteRootNode, "Note"},
         {NoteItemCountAttr, "NoteElementsCount"},
         {NoteItemNode, "NoteItem"},
         {NoteItemTypeAttr, "NoteItemType"},
         {NoteTextNode, "NoteText"},
         {NoteVoicePathNode, "NoteVoicePath"},
         {NoteVoiceSizeNode, "NoteVoiceSize"},
     };

    void parseRoot(QXmlStreamReader& xmlSRead, int& count);
    void parseNoteItem(QXmlStreamReader& xmlSRead, VNoteDatas &datas/*out*/);
};

#endif // METADATAPARSER_H
