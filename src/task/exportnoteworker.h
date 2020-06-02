/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
