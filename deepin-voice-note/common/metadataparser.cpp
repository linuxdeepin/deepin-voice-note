#include "metadataparser.h"
#include "vnoteitem.h"
#include "globaldef.h"

#include <DLog>

MetaDataParser::MetaDataParser()
{

}

void MetaDataParser::parse(const QString &metaData, VNOTE_DATAS &datas)
{
    QXmlStreamReader xmlStreamReader(metaData);

    while (xmlStreamReader.readNextStartElement()) {
        if (m_nodeNameMap[NRootNode] == xmlStreamReader.name()) {
            int dummyCount = -1;
            parseRoot(xmlStreamReader, dummyCount);

        } else if (m_nodeNameMap[NItemNode] == xmlStreamReader.name()) {
            parseNoteItem(xmlStreamReader, datas);
        } else {
            xmlStreamReader.skipCurrentElement();
        }
    }
}

void MetaDataParser::makeMetaData(const VNOTE_DATAS &datas, QString &metaData)
{
    QXmlStreamWriter xmlStreamWriter(&metaData);

    xmlStreamWriter.writeStartDocument();
    xmlStreamWriter.writeStartElement(m_nodeNameMap[NRootNode]);
    xmlStreamWriter.writeAttribute(m_nodeNameMap[NItemCountAttr], QString("%1").arg(datas.datas.length()));

    if (datas.datas.length() > 0) {
        for (auto it : datas.datas) {
            if (VNoteBlock::Text == it->getType()) {
                xmlStreamWriter.writeStartElement(m_nodeNameMap[NItemNode]);
                xmlStreamWriter.writeAttribute(m_nodeNameMap[NItemTypeAttr],
                                               QString("%1").arg(VNoteBlock::Text));

                xmlStreamWriter.writeTextElement(m_nodeNameMap[NTextNode],
                                                 it->ptrText->content);
                xmlStreamWriter.writeEndElement();
            } else if (VNoteBlock::Voice == it->getType()) {
                xmlStreamWriter.writeStartElement(m_nodeNameMap[NItemNode]);
                xmlStreamWriter.writeAttribute(m_nodeNameMap[NItemTypeAttr],
                                               QString("%1").arg(VNoteBlock::Voice));

                xmlStreamWriter.writeTextElement(m_nodeNameMap[NVoicePathNode],
                                                 it->ptrVoice->path);
                xmlStreamWriter.writeTextElement(m_nodeNameMap[NVoiceSizeNode],
                                                 QString("%1").arg(it->ptrVoice->voiceSize));
                xmlStreamWriter.writeTextElement(m_nodeNameMap[NVoiceTitleNode],
                                                 it->ptrVoice->voiceTitle);
                xmlStreamWriter.writeTextElement(m_nodeNameMap[NVoiceTextNode],
                                                 it->ptrVoice->voiceText);
                xmlStreamWriter.writeTextElement(m_nodeNameMap[NVoiceStateNode],
                                                 QString("%1").arg(it->ptrVoice->audo2TextDone));
                xmlStreamWriter.writeTextElement(m_nodeNameMap[NVoiceCreateTimeNode],
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
}

void MetaDataParser::parseRoot(QXmlStreamReader &xmlSRead, int &count)
{

    QXmlStreamAttributes noteItemCountAttr = xmlSRead.attributes();

    if (noteItemCountAttr.hasAttribute(m_nodeNameMap[NItemCountAttr])) {
        count = noteItemCountAttr.value(m_nodeNameMap[NItemCountAttr]).toInt();
//        qDebug() << "count:" << count;
    }
}

void MetaDataParser::parseNoteItem(QXmlStreamReader &xmlSRead, VNOTE_DATAS &datas)
{
    QXmlStreamAttributes noteItemTypeAttr = xmlSRead.attributes();

    int noteItemType = VNoteBlock::InValid;

    if (noteItemTypeAttr.hasAttribute(m_nodeNameMap[NItemTypeAttr])) {
        noteItemType = noteItemTypeAttr.value(m_nodeNameMap[NItemTypeAttr]).toInt();
    }

    if (noteItemType != VNoteBlock::InValid) {
        VNoteBlock* ptrBlock = nullptr;

        //Allocate block
        ptrBlock = datas.newBlock(noteItemType);

        while (xmlSRead.readNextStartElement()) {

            if (VNoteBlock::Text == noteItemType) {
                if (xmlSRead.name() == m_nodeNameMap[NTextNode]) {
                    ptrBlock->ptrText->content = xmlSRead.readElementText();
                }

            } else if (VNoteBlock::Voice == noteItemType) {
                if (xmlSRead.name() == m_nodeNameMap[NVoicePathNode]) {
                    ptrBlock->ptrVoice->path = xmlSRead.readElementText();
                } else if (xmlSRead.name() == m_nodeNameMap[NVoiceSizeNode]) {
                    ptrBlock->ptrVoice->voiceSize = QString(xmlSRead.readElementText()).toLong();
                } else if (xmlSRead.name() == m_nodeNameMap[NVoiceTitleNode]) {
                    ptrBlock->ptrVoice->voiceTitle = xmlSRead.readElementText();
                } else if (xmlSRead.name() == m_nodeNameMap[NVoiceTextNode]) {
                    ptrBlock->ptrVoice->voiceText = xmlSRead.readElementText();
                } else if (xmlSRead.name() == m_nodeNameMap[NVoiceStateNode]) {
                    ptrBlock->ptrVoice->audo2TextDone
                            = QString("%1").arg(xmlSRead.readElementText()).toInt();
                } else if (xmlSRead.name() == m_nodeNameMap[NVoiceCreateTimeNode]) {
                    ptrBlock->ptrVoice->createTime
                            = QDateTime::fromString(xmlSRead.readElementText(), VNOTE_TIME_FMT);
                }
            } else {
                xmlSRead.skipCurrentElement();
            }
        } // End while

        //Parse note item end
        datas.addBlock(ptrBlock);
    }
}
