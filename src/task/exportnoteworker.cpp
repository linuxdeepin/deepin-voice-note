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
#include "exportnoteworker.h"
#include "globaldef.h"
#include "common/vnoteitem.h"

#include <DLog>

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

/**
 * @brief ExportNoteWorker::ExportNoteWorker
 * @param dirPath 导出目录
 * @param exportType 导出类型
 * @param note 绑定记事项数据
 * @param block 绑定文本/语音
 * @param parent
 */
ExportNoteWorker::ExportNoteWorker(QString dirPath, int exportType,
                                   QList<VNoteItem *> noteList, VNoteBlock *block, QObject *parent)
    : VNTask(parent)
    , m_exportType(exportType)
    , m_exportPath(dirPath)
    //笔记列表
    , m_noteList(noteList)
    , m_noteblock(block)
{
}
/**
 * @brief ExportNoteWorker::run
 * 执行导出操作
 */
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
            //导出语音
            if (m_noteList.size() > 1) {
                exportAllVoice();
            } else {
                exportOneVoice(m_noteblock);
            }
        }
    } else {
        qCritical() << "Export note error: m_exportType=" << m_exportType;
    }

    emit exportFinished(error);
}

/**
 * @brief ExportNoteWorker::checkPath
 * @return 错误码
 */
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

/**
 * @brief ExportNoteWorker::exportText
 * @return 错误码
 * 导出文本
 */
int ExportNoteWorker::exportText()
{
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        for (auto noteData : m_noteList) {
            QString fileName = QString("%1-%2.txt").arg(noteData->noteTitle).arg(QDateTime::currentDateTime().toLocalTime().toString(VNOTE_TIME_FMT));
            QFile exportFile(m_exportPath + "/" + fileName);
            if (exportFile.open(QIODevice::ReadWrite)) {
                QTextStream stream(&exportFile);

                for (auto it : noteData->datas.datas) {
                    if (VNoteBlock::Text == it->getType()) {
                        stream << it->blockText;
                        stream << '\n';
                    }
                }

                stream.flush();
            } else {
                error = PathDenied;
            }
        }
    } else {
        error = NoteInvalid;
    }

    return error;
}

/**
 * @brief ExportNoteWorker::exportAllVoice
 * @return 错误码
 * 导出语音
 */
int ExportNoteWorker::exportAllVoice()
{
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        for (auto noteData : m_noteList) {
            for (auto it : noteData->datas.datas) {
                exportOneVoice(it);
            }
        }
    } else {
        error = NoteInvalid;
    }

    return error;
}

/**
 * @brief ExportNoteWorker::exportOneVoice
 * @param noteblock
 * @return 错误码
 */
int ExportNoteWorker::exportOneVoice(VNoteBlock *noteblock)
{
    ExportError error = ExportOK;

    if (noteblock && noteblock->blockType == VNoteBlock::Voice) {
        QFileInfo targetFile(noteblock->ptrVoice->voicePath);
        QString destFileName = m_exportPath + "/" + noteblock->ptrVoice->voiceTitle + "-" + targetFile.fileName();
        QFile::copy(noteblock->ptrVoice->voicePath, destFileName);
    } else {
        error = NoteInvalid;
    }

    return error;
}

/**
 * @brief ExportNoteWorker::exportAll
 * @return 错误码
 */
int ExportNoteWorker::exportAll()
{
    ExportError error = ExportOK;

    return error;
}
