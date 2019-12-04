#ifndef LOADNOTEITEMSWORKER_H
#define LOADNOTEITEMSWORKER_H

#include "datatypedef.h"

#include <QObject>
#include <QRunnable>
#include <QtGlobal>

class LoadNoteItemsWorker : public QObject, public QRunnable
{
public:
    LoadNoteItemsWorker(qint64 folderId, QObject *parent=nullptr);
protected:
    virtual void run();
signals:
    void onNoteItemsLoaded(VNOTE_ITEMS_MAP* foldesMap);
public slots:

protected:
    qint64 m_folderId;
};

#endif // LOADNOTEITEMSWORKER_H
