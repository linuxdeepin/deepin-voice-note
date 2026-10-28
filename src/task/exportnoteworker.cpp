// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exportnoteworker.h"
#include "ddlog.h"
#include "globaldef.h"
#include "common/vnoteitem.h"
#include "common/metadataparser.h"
#include "common/setting.h"
#include "common/utils.h"
#include "common/tiptapdocumentexporter.h"

#include <DLog>

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

/**
 * @brief ExportNoteWorker::ExportNoteWorker
 * @param dirPath 导出目录
 * @param exportType 导出类型
 * @param note 绑定记事项数据
 * @param defaultName 默认名称
 * @param parent
 */
ExportNoteWorker::ExportNoteWorker(const QString &dirPath, ExportType exportType,
                                   const QList<VNoteItem *> &noteList, const QString &defaultName, QObject *parent)
    : VNTask(parent)
    , m_exportType(exportType)
    , m_exportPath(dirPath)
    , m_exportName(defaultName)
    //笔记列表
    , m_noteList(noteList)
{
    qCInfo(voiceNoteLog) << "ExportNoteWorker constructor called with export type:" << exportType 
            << "path:" << dirPath << "default name:" << defaultName 
            << "note count:" << noteList.size();
}

/**
 * @brief ExportNoteWorker::run
 * 执行导出操作
 */
void ExportNoteWorker::run()
{
    qCInfo(voiceNoteLog) << "Starting export operation - Type:" << m_exportType;
    ExportError error = checkPath();

    if (ExportOK == error) {
        QString dirPath = m_exportPath;
        QFileInfo pathInfo(m_exportPath);
        if (pathInfo.isFile() || (!pathInfo.exists() && !m_exportPath.endsWith('/'))) {
            dirPath = pathInfo.absolutePath();
        }
        
        if (ExportText == m_exportType) {
            qCDebug(voiceNoteLog) << "Starting text export";
            error = exportText();
        } else if (ExportVoice == m_exportType) {
            qCDebug(voiceNoteLog) << "Starting voice export";
            error = exportAllVoice();
        } else if (ExportHtml == m_exportType) {
            qCDebug(voiceNoteLog) << "Starting HTML export";
            error = exportAsHtml();
        }
        
        setting::instance()->setOption(VNOTE_EXPORT_TEXT_PATH_KEY, dirPath);
        qCDebug(voiceNoteLog) << "Saved unified export directory to settings:" << dirPath;
    } else {
        qCCritical(voiceNoteLog) << "Export path check failed - Error:" << error;
    }

    qCInfo(voiceNoteLog) << "Export operation completed - Result:" << error;
    emit exportFinished(error);
    qCInfo(voiceNoteLog) << "Export operation finished signal emitted";
}

/**
 * @brief ExportNoteWorker::checkPath
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::checkPath()
{
    qCDebug(voiceNoteLog) << "Checking export path:" << m_exportPath;
    ExportError error = ExportOK;

    QFileInfo exportDir(m_exportPath);

    if (!m_exportPath.isEmpty()) {
        qCInfo(voiceNoteLog) << "m_exportPath is not empty";
        QFileInfo dir(exportDir.absolutePath());
        if (!dir.exists()) {
            qCDebug(voiceNoteLog) << "Creating export directory:" << m_exportPath;
            if (!QDir().mkpath(m_exportPath)) {
                qCWarning(voiceNoteLog) << "Failed to create export directory";
                error = PathDenied;
            }
        } else if (!dir.isWritable()) {
            qCWarning(voiceNoteLog) << "Export directory is not writable:" << m_exportPath;
            error = PathDenied;
        }
    } else {
        qCWarning(voiceNoteLog) << "Export path is empty";
        error = PathInvalid;
    }

    qCInfo(voiceNoteLog) << "Export path check finished, result:" << error;
    return error;
}

/**
 * @brief ExportNoteWorker::exportText
 * @return 错误码
 * 导出文本
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportText()
{
    qCInfo(voiceNoteLog) << "Exporting text, note count:" << m_noteList.size();
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        qCInfo(voiceNoteLog) << "m_noteList is not empty";
        for (auto noteData : m_noteList) {
            if (!noteData->haveText()) {
                qCDebug(voiceNoteLog) << "Skipping note without text content";
                continue;
            }
            QString filePath = "";
            //没有指定保存名称，则设置默认名称为笔记标题
            QFileInfo pathInfo(m_exportPath);
            if (pathInfo.isDir() || m_exportPath.endsWith('/') || m_exportName.isEmpty()) {
                QString baseFileName = m_exportPath + "/" + Utils::filteredFileName(noteData->noteTitle, "note");
                QString fileSuffix = ".txt";
                filePath = getExportFileName(baseFileName, fileSuffix);
            } else {
                filePath = m_exportPath;
            }
            QFile out(filePath);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qCWarning(voiceNoteLog) << "Failed to open file for writing:" << filePath;
                return Savefailed; //保存失败
            }
            //富文本数据需要转换为纯文本
            const QString meta = noteData->metaDataConstRef().toString();
            if (TiptapDocumentExporter::isTiptapEnvelope(meta)) {
                qCDebug(voiceNoteLog) << "Converting Tiptap envelope to plain text";
                out.write(TiptapDocumentExporter::toPlainText(meta).toUtf8());
            } else if (!noteData->htmlCode.isEmpty()) {
                qCDebug(voiceNoteLog) << "Converting HTML content to plain text";
                QTextDocument doc;
                doc.setHtml(noteData->htmlCode);
                out.write(doc.toPlainText().toUtf8());
            } else {
                qCDebug(voiceNoteLog) << "Writing plain text content";
                for (auto it : noteData->datas.datas) {
                    if (VNoteBlock::Text == it->getType()) {
                        out.write(it->blockText.toUtf8());
                        out.write("\n");
                    }
                }
            }
            out.close();
            qCInfo(voiceNoteLog) << "Successfully exported text to:" << filePath;
        }
    } else {
        qCWarning(voiceNoteLog) << "No notes to export";
        error = NoteInvalid;
    }
    qCInfo(voiceNoteLog) << "Text export finished, result:" << error;
    return error;
}

/**
 * @brief ExportNoteWorker::exportAllVoice
 * @return 错误码
 * 导出语音
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportAllVoice()
{
    qCInfo(voiceNoteLog) << "Exporting all voice, note count:" << m_noteList.size();
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        qCInfo(voiceNoteLog) << "m_noteList is not empty";
        for (auto noteData : m_noteList) {
            if (!noteData->haveVoice()) {
                qCDebug(voiceNoteLog) << "Skipping note without voice content";
                continue;
            }
            const QString meta = noteData->metaDataConstRef().toString();
            if (TiptapDocumentExporter::isTiptapEnvelope(meta)) {
                qCDebug(voiceNoteLog) << "Processing Tiptap note voice blocks";
                for (const QString &voiceJson : TiptapDocumentExporter::voiceJsons(meta)) {
                    error = exportOneVoice(voiceJson);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        qCWarning(voiceNoteLog) << "Failed to export Tiptap voice block from JSON";
                        return error;
                    }
                }
            } else if (noteData->htmlCode.isEmpty()) {
                qCDebug(voiceNoteLog) << "Processing plain text note voice blocks";
                for (auto it : noteData->datas.datas) {
                    error = exportOneVoice(it);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        qCWarning(voiceNoteLog) << "Failed to export voice block";
                        return error;
                    }
                }
            } else {
                //富文本笔记
                qCDebug(voiceNoteLog) << "Processing rich text note voice blocks";
                for (auto it : noteData->getVoiceJsons()) {
                    error = exportOneVoice(it);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        qCWarning(voiceNoteLog) << "Failed to export voice block from JSON";
                        return error;
                    }
                }
            }
        }
    } else {
        qCWarning(voiceNoteLog) << "No notes to export";
        error = NoteInvalid;
    }

    qCInfo(voiceNoteLog) << "All voice export finished, result:" << error;
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
    qCInfo(voiceNoteLog) << "Exporting one voice from JSON";
    VNVoiceBlock voiceBlock;
    MetaDataParser dataParser;
    //解析json数据
    if (dataParser.parse(json, &voiceBlock)) {
        qCInfo(voiceNoteLog) << "Voice export from JSON success";
        return exportOneVoice(&voiceBlock);
    }
    qCInfo(voiceNoteLog) << "Voice export from JSON failed, invalid data";
    return NoteInvalid;
}

/**
 * @brief ExportNoteWorker::exportOneVoice
 * @param noteblock
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportOneVoice(VNoteBlock *noteblock)
{
    qCDebug(voiceNoteLog) << "Exporting voice block";
    ExportError error = ExportOK;

    if (noteblock && noteblock->blockType == VNoteBlock::Voice) {
        qCInfo(voiceNoteLog) << "noteblock is not nullptr and noteblock->blockType is Voice";
        QString baseFileName = m_exportPath + "/" + noteblock->ptrVoice->voiceTitle;
        QString fileSuffix = ".mp3";
        QString dstFileName = getExportFileName(baseFileName, fileSuffix);
        const QString srcPath = Utils::makeVoiceAbsolute(noteblock->ptrVoice->voicePath);
        qCDebug(voiceNoteLog) << "Copying voice file from:" << srcPath << "to:" << dstFileName;
        if (!QFile::copy(srcPath, dstFileName)) {
            qCWarning(voiceNoteLog) << "Failed to copy voice file from:" << srcPath;
            error = Savefailed; //保存失败
        } else {
            qCInfo(voiceNoteLog) << "Successfully exported voice to:" << dstFileName;
        }
    } else {
        qCWarning(voiceNoteLog) << "Invalid voice block";
        error = NoteInvalid;
    }

    qCInfo(voiceNoteLog) << "Voice block export finished, result:" << error;
    return error;
}

/**
 * @brief ExportNoteWorker::exportAsHtml
 * 将笔记导出为Html
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportAsHtml()
{
    qCInfo(voiceNoteLog) << "Exporting as HTML, note count:" << m_noteList.size();
    ExportError error = ExportOK;
    for (auto note : m_noteList) {
        if (!note->haveText()) {
            qCDebug(voiceNoteLog) << "Skipping note without text content";
            continue;
        }
        QString filePath = "";
        //没有指定保存名称，则设置默认名称为笔记标题
        QFileInfo pathInfo(m_exportPath);
        if (pathInfo.isDir() || m_exportPath.endsWith('/') || m_exportName.isEmpty()) {
            QString baseFileName = m_exportPath + "/" + Utils::filteredFileName(note->noteTitle, "note");
            QString fileSuffix = ".html";
            filePath = getExportFileName(baseFileName, fileSuffix);
        } else {            
            filePath = m_exportPath;
        }
        QFile out(filePath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCWarning(voiceNoteLog) << "Failed to open file for writing:" << filePath;
            return Savefailed; //保存失败
        }
        if (!out.write(note->getFullHtml().toUtf8())) {
            qCWarning(voiceNoteLog) << "Failed to write HTML content to file:" << filePath;
            return Savefailed; //保存失败
        }
        out.close();  
    }
    qCInfo(voiceNoteLog) << "HTML export finished, result:" << error;
    return error;
}

QString ExportNoteWorker::getExportFileName(const QString &baseName, const QString &fileSuffix)
{
    qCDebug(voiceNoteLog) << "Generating unique filename for:" << baseName << fileSuffix;
    QString filePath = baseName + fileSuffix;
    int count = 1;
    while (1) {
        QFile file(filePath);
        if (!file.exists()) {
            break;
        }
        filePath = baseName + "(" + QString::number(count++) + ")" + fileSuffix;
    }
    qCInfo(voiceNoteLog) << "Generated unique filename:" << filePath;
    return filePath;
}
