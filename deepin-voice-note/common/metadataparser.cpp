#include "metadataparser.h"
#include "vnoteitem.h"

#include <DLog>

MetaDataParser::MetaDataParser()
{

}

void MetaDataParser::parse(const QString &metaData, VNoteDatas &datas)
{
    QXmlStreamReader xmlStreamReader(metaData);

    while (xmlStreamReader.readNextStartElement()) {
        if (m_nodeNameMap[NoteRootNode] == xmlStreamReader.name()) {
            int count = -1;
            parseRoot(xmlStreamReader, count);

        } else if (m_nodeNameMap[NoteItemNode] == xmlStreamReader.name()) {
            parseNoteItem(xmlStreamReader, datas);
        } else {
            xmlStreamReader.skipCurrentElement();
        }
    }
}

void MetaDataParser::makeMetaData(const VNoteDatas &datas, QString &metaData)
{
    QXmlStreamWriter xmlStreamWriter(&metaData);

    xmlStreamWriter.writeStartDocument();
    xmlStreamWriter.writeStartElement(m_nodeNameMap[NoteRootNode]);
    xmlStreamWriter.writeAttribute(m_nodeNameMap[NoteItemCountAttr], QString("%1").arg(datas.length()));

    if (datas.length() > 0) {
        for (auto it : datas) {
            if (VNoteBlock::Text == it->getType()) {
                xmlStreamWriter.writeStartElement(m_nodeNameMap[NoteItemNode]);
                xmlStreamWriter.writeAttribute(m_nodeNameMap[NoteItemTypeAttr],
                                               QString("%1").arg(VNoteBlock::Text));

                xmlStreamWriter.writeTextElement(m_nodeNameMap[NoteTextNode],
                                                 it->ptrText->content);
                xmlStreamWriter.writeEndElement();
            } else if (VNoteBlock::Voice == it->getType()) {
                xmlStreamWriter.writeStartElement(m_nodeNameMap[NoteItemNode]);
                xmlStreamWriter.writeAttribute(m_nodeNameMap[NoteItemTypeAttr],
                                               QString("%1").arg(VNoteBlock::Voice));

                xmlStreamWriter.writeTextElement(m_nodeNameMap[NoteVoicePathNode],
                                                 it->ptrVoice->path);
                xmlStreamWriter.writeTextElement(m_nodeNameMap[NoteVoiceSizeNode],
                                                 QString("%1").arg(it->ptrVoice->voiceSize));

                xmlStreamWriter.writeEndElement();
            } else {

            }
        }
    }

    xmlStreamWriter.writeEndElement();
    xmlStreamWriter.writeEndDocument();
}

void MetaDataParser::parseRoot(QXmlStreamReader &xmlSRead, int &count)
{

    QXmlStreamAttributes noteItemCountAttr = xmlSRead.attributes();

    if (noteItemCountAttr.hasAttribute(m_nodeNameMap[NoteItemCountAttr])) {
        count = noteItemCountAttr.value(m_nodeNameMap[NoteItemCountAttr]).toInt();
    }
}

void MetaDataParser::parseNoteItem(QXmlStreamReader &xmlSRead, VNoteDatas &datas)
{
    QXmlStreamAttributes noteItemTypeAttr = xmlSRead.attributes();

    int noteItemType = VNoteBlock::InValid;

    if (noteItemTypeAttr.hasAttribute(m_nodeNameMap[NoteItemTypeAttr])) {
        noteItemType = noteItemTypeAttr.value(m_nodeNameMap[NoteItemTypeAttr]).toInt();
    }

    if (noteItemType != VNoteBlock::InValid) {
        VNoteBlock* ptrBlock = nullptr;
        if (VNoteBlock::Text == noteItemType) {
            ptrBlock = new VNTextBlock();
        } else if (VNoteBlock::Voice == noteItemType) {
            ptrBlock = new VNVoiceBlock();
        }

        while (xmlSRead.readNextStartElement()) {

            if (VNoteBlock::Text == noteItemType) {
                if (xmlSRead.name() == m_nodeNameMap[NoteTextNode]) {
                    ptrBlock->ptrText->content = xmlSRead.readElementText();
                }

            } else if (VNoteBlock::Voice == noteItemType) {
                if (xmlSRead.name() == m_nodeNameMap[NoteVoicePathNode]) {
                    ptrBlock->ptrVoice->path = xmlSRead.readElementText();
                } else if (xmlSRead.name() == m_nodeNameMap[NoteVoiceSizeNode]) {
                    ptrBlock->ptrVoice->voiceSize = QString(xmlSRead.readElementText()).toLong();
                }
            } else {
                xmlSRead.skipCurrentElement();
            }
        }

        datas.push_back(ptrBlock);
    }
}
