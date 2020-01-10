#include "vnoteitem.h"

VNoteItem::VNoteItem()
{

}

bool VNoteItem::isValid()
{
    return (noteId > INVALID_ID
            && folderId > INVALID_ID) ? true : false;
}

VNoteItemWidget::VNoteItemWidget(QWidget *parent)
    : DWidget(parent)
{
    ;
}

VNoteItemWidget::~VNoteItemWidget()
{
    ;
}

VNoteItem *VNoteItemWidget::getNoteItem()
{
    return  nullptr;
}
