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

void VNoteItem::delNoteData()
{
    //Clear note attachments
    for (auto it : datas.datas) {
        it->releaseSpecificData();
    }
}

//bool VNoteItem::makeMetaData()
//{
//    bool isMetaDataOk = false;

//    return isMetaDataOk;
//}

void VNoteItem::setMetadata(const QString &meta)
{
    metaData = meta;
}

QDebug& operator << (QDebug &out, VNoteItem &noteItem)
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

VNoteBlock::~VNoteBlock()
{

}

qint32 VNoteBlock::getType()
{
    return blockType;
}

VNTextBlock::VNTextBlock()
    :VNoteBlock(Text)
{
}

VNTextBlock::~VNTextBlock()
{

}

void VNTextBlock::releaseSpecificData()
{
    //TODO:
    //    Add text specific operation code here.
    //
    //Do nothing for text now.

}

VNVoiceBlock::VNVoiceBlock()
    :VNoteBlock(Voice)
{
    blockType = Voice;
}

VNVoiceBlock::~VNVoiceBlock()
{
}

void VNVoiceBlock::releaseSpecificData()
{
    //TODO:
    //    Add voice specific operation code here:
    qInfo() << "Remove file:" << path;

    QFileInfo fileInfo(path);

    if (fileInfo.exists()) {
        QFile::remove(path);
    }
}
