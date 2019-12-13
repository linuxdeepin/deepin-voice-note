#ifndef LOADNOTEITEMSWORKER_H
#define LOADNOTEITEMSWORKER_H

#include "datatypedef.h"

#include <QObject>
#include <QRunnable>
#include <QtGlobal>

class LoadNoteItemsWorker : public QObject, public QRunnable
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
