#include "loadnoteitemsworker.h"

LoadNoteItemsWorker::LoadNoteItemsWorker(qint64 folderId,QObject *parent)
    :QObject(parent)
    ,QRunnable ()
    ,m_folderId(folderId)
{

}

void LoadNoteItemsWorker::run()
{

}
