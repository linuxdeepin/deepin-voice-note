#ifndef EXPORTNOTEWORKER_H
#define EXPORTNOTEWORKER_H

#include "datatypedef.h"

#include <QObject>
#include <QRunnable>

class ExportNoteWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ExportNoteWorker(QString dirPath,
                              int exportType,
                              VNoteItem* note,
                              QObject *parent = nullptr);
    enum {
        ExportNothing,
        ExportText,
        ExportVoice,
        ExportAll,
    };

    enum ExportError {
        ExportOK,
        NoteInvalid,
        PathDenied,
        PathInvalid,
    };

signals:
    void exportFinished(int state);
public slots:
protected:
    virtual void run() override;

    int checkPath();
    int exportText();
    int exportVoice();
    int exportAll();

    int        m_exportType {ExportNothing};
    QString    m_exportPath;
    VNoteItem* m_note {nullptr};
};

#endif // EXPORTNOTEWORKER_H
