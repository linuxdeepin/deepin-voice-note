#ifndef EXPORTNOTEWORKER_H
#define EXPORTNOTEWORKER_H

#include "common/datatypedef.h"
#include "vntask.h"

#include <QObject>
#include <QRunnable>

class ExportNoteWorker : public VNTask
{
    Q_OBJECT
public:
    explicit ExportNoteWorker(QString dirPath,
                              int exportType,
                              VNoteItem* note,
                              VNoteBlock* block = nullptr,
                              QObject *parent = nullptr);
    enum {
        ExportNothing,
        ExportText,
        ExportAllVoice,
        ExportAll,
        ExportOneVoice
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
    int exportAllVoice();
    int exportAll();
    int exportOneVoice(VNoteBlock* block);

    int        m_exportType {ExportNothing};
    QString    m_exportPath;
    VNoteItem* m_note {nullptr};
    VNoteBlock* m_noteblock {nullptr};
};

#endif // EXPORTNOTEWORKER_H
