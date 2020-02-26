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
            && folderId > INVALID_ID) ? true : false;
}

void VNoteItem::delVoiceFile()
{
    if (VNT_Voice == noteType) {
        qInfo() << "Remove file:" << voicePath;

        QFileInfo fileInfo(voicePath);

        if (fileInfo.exists()) {
            QFile::remove(voicePath);
        }
    }
}

bool VNoteItem::makeMetaData()
{
    bool isMetaDataOk = false;

    return isMetaDataOk;
}

QDebug& operator << (QDebug &out, VNoteItem &noteItem)
{
    out << "\n{ "
        << "noteId=" << noteItem.noteId << ","
        << "folderId=" << noteItem.folderId << ","
        << "noteType=" << noteItem.noteType << ","
        << "voiceSize=" << noteItem.voiceSize << ","
        << "noteState=" << noteItem.noteState << ","
        << "noteTitle=" << noteItem.noteTitle << ","
        << "noteText=" << noteItem.noteText << ","
        << "voicePath=" << noteItem.voicePath << ","
        << "createTime=" << noteItem.createTime << ","
        << "modifyTime=" << noteItem.modifyTime << ","
        << "deleteTime=" << noteItem.deleteTime
        << " }\n";

    return out;
}

VNoteItemWidget::VNoteItemWidget(QWidget *parent)
    : DWidget(parent)
{
    ;
}

VNoteBlock::VNoteBlock(qint32 type)
    :blockType(type)
{
    ptrBlock = this;
}

qint32 VNoteBlock::getType()
{
    return blockType;
}

VNTextBlock::VNTextBlock()
    :VNoteBlock(Text)
{
}

VNVoiceBlock::VNVoiceBlock()
    :VNoteBlock(Voice)
{
    blockType = Voice;
}
