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

VNoteItemWidget::VNoteItemWidget(QWidget *parent)
    : DWidget(parent)
{
    ;
}

VNoteItem *VNoteItemWidget::getNoteItem()
{
    return  nullptr;
}
