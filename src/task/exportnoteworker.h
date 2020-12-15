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

//一个记事项文本，语音导出线程
class ExportNoteWorker : public VNTask
{
    Q_OBJECT
public:
    explicit ExportNoteWorker(QString dirPath,
                              int exportType,
                              QList<VNoteItem *>noteList,
                              VNoteBlock *block = nullptr,
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
    //导出完成信号
    void exportFinished(int state);
public slots:
protected:
    virtual void run() override;
    //检查路径
    int checkPath();
    //导出文本
    int exportText();
    //导出所有语音
    int exportAllVoice();
    //导出所有文本和语音
    int exportAll();
    //导出一个语音
    int exportOneVoice(VNoteBlock *block);

    int m_exportType {ExportNothing};
    QString m_exportPath;
    //导出语音
    QList<VNoteItem *>m_noteList {nullptr};
    VNoteBlock *m_noteblock {nullptr};
};

#endif // EXPORTNOTEWORKER_H
