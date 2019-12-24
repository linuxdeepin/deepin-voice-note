#include "loadnoteitemsworker.h"
#include "db/vnoteitemoper.h"
#include "globaldef.h"

#include <DLog>

LoadNoteItemsWorker::LoadNoteItemsWorker(QObject *parent)
    :QObject(parent)
    ,QRunnable ()
{

}

void LoadNoteItemsWorker::run()
{
    static struct timeval start,backups, end;

    gettimeofday(&start, nullptr);
    backups = start;

    VNoteItemOper notesOper;
    VNOTE_ALL_NOTES_MAP* notesMap = notesOper.loadAllVNotes();

    gettimeofday(&end, nullptr);

    qDebug() << "LoadNoteItemsWorker(ms):" << TM(start, end);

    //TODO:
    //    Add load folder code here

    qDebug() << __FUNCTION__ << " load all notes ok:" << notesMap->notes.size() << " thread id:" << QThread::currentThreadId();

    emit onAllNotesLoaded(notesMap);
}
