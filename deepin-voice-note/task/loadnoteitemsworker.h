#ifndef LOADNOTEITEMSWORKER_H
#define LOADNOTEITEMSWORKER_H

#include "common/datatypedef.h"
#include "vntask.h"

#include <QObject>
#include <QRunnable>
#include <QtGlobal>

class LoadNoteItemsWorker : public VNTask
{
    Q_OBJECT
public:
    LoadNoteItemsWorker(QObject *parent=nullptr);
protected:
    virtual void run();
signals:
    void onAllNotesLoaded(VNOTE_ALL_NOTES_MAP* foldesMap);
public slots:

protected:
};

#endif // LOADNOTEITEMSWORKER_H
