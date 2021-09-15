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
#include "common/metadataparser.h"

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
    ExportError error = checkPath();

    if (ExportOK == error) {
        if (ExportText == m_exportType) {
            error = exportText();
        } else if (ExportAllVoice == m_exportType) {
            error = exportAllVoice();
        } else if (ExportOneVoice == m_exportType) {
            //导出语音
            if (m_noteList.size() > 1) {
                error = exportAllVoice();
            } else {
                error = exportOneVoice(m_noteblock);
            }
        } else if (ExportHtml == m_exportType) {
            error = exportAsHtml();
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
ExportNoteWorker::ExportError ExportNoteWorker::checkPath()
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
ExportNoteWorker::ExportError ExportNoteWorker::exportText()
{
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        for (auto noteData : m_noteList) {
            QString fileName = QString("%1-%2.txt").arg(noteData->noteTitle).arg(QDateTime::currentDateTime().toLocalTime().toString(VNOTE_TIME_FMT));
            QFile exportFile(m_exportPath + "/" + fileName);
            if (!exportFile.open(QIODevice::ReadWrite)) {
                return Savefailed;
            }
            QTextStream stream(&exportFile);
            //富文本数据需要转换为纯文本
            if (!noteData->htmlCode.isEmpty()) {
                QTextDocument doc;
                doc.setHtml(noteData->htmlCode);
                stream << doc.toPlainText();
            } else {
                for (auto it : noteData->datas.datas) {
                    if (VNoteBlock::Text == it->getType()) {
                        stream << it->blockText;
                        stream << '\n';
                    }
                }
            }
            stream.flush();
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
ExportNoteWorker::ExportError ExportNoteWorker::exportAllVoice()
{
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        for (auto noteData : m_noteList) {
            if (noteData->htmlCode.isEmpty()) {
                for (auto it : noteData->datas.datas) {
                    error = exportOneVoice(it);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        return error;
                    }
                }
            } else {
                //富文本笔记
                for (auto it : noteData->getVoiceJsons()) {
                    error = exportOneVoice(it);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        return error;
                    }
                }
            }
        }
    } else {
        error = NoteInvalid;
    }

    return error;
}

/**
 * @brief ExportNoteWorker::exportOneVoice
 * 导出语音
 * @param json json格式的语音数据
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportOneVoice(const QString &json)
{
    VNVoiceBlock voiceBlock;
    MetaDataParser dataParser;
    //解析json数据
    if (dataParser.parse(json, &voiceBlock)) {
        return exportOneVoice(&voiceBlock);
    }
    return NoteInvalid;
}

/**
 * @brief ExportNoteWorker::exportOneVoice
 * @param noteblock
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportOneVoice(VNoteBlock *noteblock)
{
    ExportError error = ExportOK;

    if (noteblock && noteblock->blockType == VNoteBlock::Voice) {
        QFileInfo targetFile(noteblock->ptrVoice->voicePath);
        QString destFileName = m_exportPath + "/" + noteblock->ptrVoice->voiceTitle + "-" + targetFile.fileName();
        if (!QFile::copy(noteblock->ptrVoice->voicePath, destFileName)) {
            error = Savefailed; //保存失败
        }
    } else {
        error = NoteInvalid;
    }

    return error;
}

/**
 * @brief ExportNoteWorker::exportAsHtml
 * 将笔记导出为Html
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportAsHtml()
{
    ExportError error = ExportOK;
    for (auto note : m_noteList) {
        //构建文件名
        QString fileName = QString("%1-%2.html").arg(note->noteTitle).arg(QDateTime::currentDateTime().toLocalTime().toString("yyyyMMddhhmmss"));
        QFile out(m_exportPath + "/" + fileName);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return Savefailed; //保存失败
        }
        if (!out.write(note->getFullHtml().toUtf8())) {
            return Savefailed; //保存失败
        }
    }
    return error;
}
