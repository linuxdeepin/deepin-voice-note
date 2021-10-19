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
    enum ExportType {
        ExportNothing,
        ExportText,
        ExportVoice,
        ExportHtml, //导出为html
    };

    enum ExportError {
        ExportOK,
        NoteInvalid,
        PathDenied,
        PathInvalid,
        Savefailed, //保存失败
    };

    explicit ExportNoteWorker(const QString &dirPath,
                              ExportType exportType,
                              const QList<VNoteItem *> &noteList,
                              const QString &defaultName = "",
                              QObject *parent = nullptr);

signals:
    //导出完成信号
    void exportFinished(int state);
public slots:
protected:
    virtual void run() override;
    //检查路径
    ExportError checkPath();
    //导出文本
    ExportError exportText();
    //导出所有语音
    ExportError exportAllVoice();
    //导出语音
    ExportError exportOneVoice(VNoteBlock *block);
    ExportError exportOneVoice(const QString &);
    //导出为HTML
    ExportError exportAsHtml();
    //获取导出文件名，同名时创建副本
    QString getExportFileName(const QString &baseName, const QString &fileSuffix);
    //过滤文件名内容
    QString filteredFileName(QString fileName);

    ExportType m_exportType {ExportNothing};
    QString m_exportPath {""};
    //默认导出名称
    QString m_exportName {""};
    QList<VNoteItem *> m_noteList {nullptr};
};

#endif // EXPORTNOTEWORKER_H
