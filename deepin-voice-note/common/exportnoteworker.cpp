#include "exportnoteworker.h"
#include "globaldef.h"
#include "common/vnoteitem.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

#include <DLog>

ExportNoteWorker::ExportNoteWorker(QString dirPath, int exportType,
                                   VNoteItem* note, VNoteBlock* block, QObject *parent)
    : QObject(parent)
    ,QRunnable ()
    ,m_exportType(exportType)
    ,m_exportPath(dirPath)
    ,m_note(note)
    ,m_noteblock(block)
{

}

void ExportNoteWorker::run()
{
    ExportError error = static_cast<ExportError>(checkPath());

    if (ExportOK == error) {
        if (ExportText == m_exportType) {
            exportText();
        } else if (ExportAllVoice == m_exportType) {
            exportAllVoice();
        } else if (ExportAll == m_exportType) {
            exportAll();
        } else if (ExportOneVoice == m_exportType) {
            exportOneVoice(m_noteblock);
        }
    } else {
        qCritical() << "Export note error: m_exportType=" << m_exportType
                    << " error:" << error
                    << " " << *m_note;
    }

    emit exportFinished(error);
}

int ExportNoteWorker::checkPath()
{
    ExportError error = ExportOK;

    QFileInfo exportDir(m_exportPath);

    if (!m_exportPath.isEmpty()) {
        if (!exportDir.exists()) {
            if (!QDir().mkpath(m_exportPath)) {
                error = PathDenied;
            }
        } else if (!exportDir.isWritable()) {
            error = PathDenied;
        }
    } else {
        error = PathInvalid;
    }

    return error;
}

int ExportNoteWorker::exportText()
{
    ExportError error = ExportOK;

    if (nullptr != m_note) {
        QString fileName = QString("%1-%2.txt").arg(m_note->noteTitle)
                .arg(QDateTime::currentDateTime().toLocalTime().toString(VNOTE_TIME_FMT));

        QFile exportFile(m_exportPath+"/"+fileName);

        if (exportFile.open(QIODevice::ReadWrite)) {
            QTextStream stream( &exportFile);

            for (auto it : m_note->datas.datas) {
                if (VNoteBlock::Text == it->getType()) {
                    stream << it->blockText;
                }
            }

            stream.flush();
        } else {
            error = PathDenied;
        }
    } else {
        error = NoteInvalid;
    }

    return  error;
}

int ExportNoteWorker::exportAllVoice()
{
    ExportError error = ExportOK;

    if (nullptr != m_note) {
        for (auto it : m_note->datas.datas) {
            exportOneVoice(it);
        }
    } else {
        error = NoteInvalid;
    }

    return  error;
}

int ExportNoteWorker::exportOneVoice(VNoteBlock *noteblock)
{
    ExportError error = ExportOK;

    if(noteblock &&
       noteblock->blockType == VNoteBlock::Voice){
       QFileInfo targetFile(noteblock->ptrVoice->voicePath);
       QString destFileName = m_exportPath + "/" +
               noteblock->ptrVoice->voiceTitle + "-" + targetFile.fileName();
       QFile::copy(noteblock->ptrVoice->voicePath, destFileName);
    }else {
        error = NoteInvalid;
    }

    return  error;
}

int ExportNoteWorker::exportAll()
{
    ExportError error = ExportOK;

    return  error;
}
