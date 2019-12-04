#include "vnoteforlder.h"

VNoteFolder::VNoteFolder()
{
}

VNoteFolder::~VNoteFolder()
{
}

bool VNoteFolder::isValid()
{
    return (id > INVALID_ID) ? true : false;
}
