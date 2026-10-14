// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exportnoteworker.h"
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextDocument>

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
    qInfo() << "ExportNoteWorker constructor called with export type:" << exportType 
            << "path:" << dirPath << "default name:" << defaultName 
            << "note count:" << noteList.size();
}

/**
 * @brief ExportNoteWorker::run
 * 执行导出操作
 */
void ExportNoteWorker::run()
{
    qInfo() << "Starting export operation - Type:" << m_exportType;
    ExportError error = checkPath();

    if (ExportOK == error) {
        QString dirPath = m_exportPath;
        QFileInfo pathInfo(m_exportPath);
        if (pathInfo.isFile() || (!pathInfo.exists() && !m_exportPath.endsWith('/'))) {
            dirPath = pathInfo.absolutePath();
        }
        
        if (ExportText == m_exportType) {
            qDebug() << "Starting text export";
            error = exportText();
        } else if (ExportVoice == m_exportType) {
            qDebug() << "Starting voice export";
            error = exportAllVoice();
        } else if (ExportHtml == m_exportType) {
            qDebug() << "Starting HTML export";
            error = exportAsHtml();
        }
        
        setting::instance()->setOption(VNOTE_EXPORT_TEXT_PATH_KEY, dirPath);
        qDebug() << "Saved unified export directory to settings:" << dirPath;
    } else {
        qCritical() << "Export path check failed - Error:" << error;
    }

    qInfo() << "Export operation completed - Result:" << error;
    emit exportFinished(error);
    qInfo() << "Export operation finished signal emitted";
}

/**
 * @brief ExportNoteWorker::checkPath
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::checkPath()
{
    qDebug() << "Checking export path:" << m_exportPath;
    ExportError error = ExportOK;

    QFileInfo exportDir(m_exportPath);

    if (!m_exportPath.isEmpty()) {
        qInfo() << "m_exportPath is not empty";
        QFileInfo dir(exportDir.absolutePath());
        if (!dir.exists()) {
            qDebug() << "Creating export directory:" << m_exportPath;
            if (!QDir().mkpath(m_exportPath)) {
                qWarning() << "Failed to create export directory";
                error = PathDenied;
            }
        } else if (!dir.isWritable()) {
            qWarning() << "Export directory is not writable:" << m_exportPath;
            error = PathDenied;
        }
    } else {
        qWarning() << "Export path is empty";
        error = PathInvalid;
    }

    qInfo() << "Export path check finished, result:" << error;
    return error;
}

/**
 * @brief ExportNoteWorker::exportText
 * @return 错误码
 * 导出文本
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportText()
{
    qInfo() << "Exporting text, note count:" << m_noteList.size();
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        qInfo() << "m_noteList is not empty";
        for (auto noteData : m_noteList) {
            if (!noteData->haveText()) {
                qDebug() << "Skipping note without text content";
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
                qWarning() << "Failed to open file for writing:" << filePath;
                return Savefailed; //保存失败
            }
            //富文本数据需要转换为纯文本
            const QString meta = noteData->metaDataConstRef().toString();
            if (TiptapDocumentExporter::isTiptapEnvelope(meta)) {
                qDebug() << "Converting Tiptap envelope to plain text";
                out.write(TiptapDocumentExporter::toPlainText(meta).toUtf8());
            } else if (!noteData->htmlCode.isEmpty()) {
                qDebug() << "Converting HTML content to plain text";
                QString htmlCode = stripVoiceUiElements(noteData->htmlCode);
                QTextDocument doc;
                doc.setHtml(htmlCode);
                out.write(doc.toPlainText().toUtf8());
            } else {
                qDebug() << "Writing plain text content";
                for (auto it : noteData->datas.datas) {
                    if (VNoteBlock::Text == it->getType()) {
                        out.write(it->blockText.toUtf8());
                        out.write("\n");
                    }
                }
            }
            out.close();
            qInfo() << "Successfully exported text to:" << filePath;
        }
    } else {
        qWarning() << "No notes to export";
        error = NoteInvalid;
    }
    qInfo() << "Text export finished, result:" << error;
    return error;
}

/**
 * @brief ExportNoteWorker::exportAllVoice
 * @return 错误码
 * 导出语音
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportAllVoice()
{
    qInfo() << "Exporting all voice, note count:" << m_noteList.size();
    ExportError error = ExportOK;
    //存在note
    if (m_noteList.size()) {
        qInfo() << "m_noteList is not empty";
        for (auto noteData : m_noteList) {
            if (!noteData->haveVoice()) {
                qDebug() << "Skipping note without voice content";
                continue;
            }
            const QString meta = noteData->metaDataConstRef().toString();
            if (TiptapDocumentExporter::isTiptapEnvelope(meta)) {
                qDebug() << "Processing Tiptap note voice blocks";
                for (const QString &voiceJson : TiptapDocumentExporter::voiceJsons(meta)) {
                    error = exportOneVoice(voiceJson);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        qWarning() << "Failed to export Tiptap voice block from JSON";
                        return error;
                    }
                }
            } else if (noteData->htmlCode.isEmpty()) {
                qDebug() << "Processing plain text note voice blocks";
                for (auto it : noteData->datas.datas) {
                    error = exportOneVoice(it);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        qWarning() << "Failed to export voice block";
                        return error;
                    }
                }
            } else {
                //富文本笔记
                qDebug() << "Processing rich text note voice blocks";
                for (auto it : noteData->getVoiceJsons()) {
                    error = exportOneVoice(it);
                    //某一个保存失败则后续的不再进行保存操作
                    if (Savefailed == error) {
                        qWarning() << "Failed to export voice block from JSON";
                        return error;
                    }
                }
            }
        }
    } else {
        qWarning() << "No notes to export";
        error = NoteInvalid;
    }

    qInfo() << "All voice export finished, result:" << error;
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
    qInfo() << "Exporting one voice from JSON";
    VNVoiceBlock voiceBlock;
    MetaDataParser dataParser;
    //解析json数据
    if (dataParser.parse(json, &voiceBlock)) {
        qInfo() << "Voice export from JSON success";
        return exportOneVoice(&voiceBlock);
    }
    qInfo() << "Voice export from JSON failed, invalid data";
    return NoteInvalid;
}

/**
 * @brief ExportNoteWorker::exportOneVoice
 * @param noteblock
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportOneVoice(VNoteBlock *noteblock)
{
    qDebug() << "Exporting voice block";
    ExportError error = ExportOK;

    if (noteblock && noteblock->blockType == VNoteBlock::Voice) {
        qInfo() << "noteblock is not nullptr and noteblock->blockType is Voice";
        QString baseFileName = m_exportPath + "/" + noteblock->ptrVoice->voiceTitle;
        QString fileSuffix = ".mp3";
        QString dstFileName = getExportFileName(baseFileName, fileSuffix);
        const QString srcPath = Utils::makeVoiceAbsolute(noteblock->ptrVoice->voicePath);
        qDebug() << "Copying voice file from:" << srcPath << "to:" << dstFileName;
        if (!QFile::copy(srcPath, dstFileName)) {
            qWarning() << "Failed to copy voice file from:" << srcPath;
            error = Savefailed; //保存失败
        } else {
            qInfo() << "Successfully exported voice to:" << dstFileName;
        }
    } else {
        qWarning() << "Invalid voice block";
        error = NoteInvalid;
    }

    qInfo() << "Voice block export finished, result:" << error;
    return error;
}

/**
 * @brief ExportNoteWorker::exportAsHtml
 * 将笔记导出为Html
 * @return 错误码
 */
ExportNoteWorker::ExportError ExportNoteWorker::exportAsHtml()
{
    qInfo() << "Exporting as HTML, note count:" << m_noteList.size();
    ExportError error = ExportOK;
    for (auto note : m_noteList) {
        if (!note->haveText()) {
            qDebug() << "Skipping note without text content";
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
            qWarning() << "Failed to open file for writing:" << filePath;
            return Savefailed; //保存失败
        }
        if (!out.write(note->getFullHtml().toUtf8())) {
            qWarning() << "Failed to write HTML content to file:" << filePath;
            return Savefailed; //保存失败
        }
        out.close();  
    }
    qInfo() << "HTML export finished, result:" << error;
    return error;
}

/**
 * @brief ExportNoteWorker::stripVoiceUiElements
 * 剥离语音块中的 UI 元素，从 jsonKey 属性中恢复转写文字。
 *
 * 导出 TXT 时，htmlCode 中的语音块包含 "正在转为文字" 占位文本、
 * 时间字段、播放按钮等 UI 元素，而实际的转写文字仅存储在 jsonKey
 * 的 JSON 属性中。本函数将整个语音块 div 替换为从 JSON 中提取的
 * 转写文字（经过 HTML 转义），若 JSON 中无 text 字段则移除整个语音块。
 *
 * @param html 原始 HTML 字符串
 * @return 处理后的 HTML 字符串
 */
QString ExportNoteWorker::stripVoiceUiElements(const QString &html)
{
    static const QRegularExpression voiceBoxRx(
        QStringLiteral(R"(<div[^>]*\bjsonKey\s*=\s*"([^"]*)"[^>]*>)"),
        QRegularExpression::CaseInsensitiveOption);

    QString result = html;
    int searchOffset = 0;

    while (true) {
        QRegularExpressionMatch match = voiceBoxRx.match(result, searchOffset);
        if (!match.hasMatch()) {
            break;
        }

        int divStart = match.capturedStart(0);
        int divContentStart = match.capturedEnd(0);
        QString jsonKeyRaw = match.captured(1);

        // Find the matching closing </div> using depth counting
        int depth = 1;
        int pos = divContentStart;
        static const QRegularExpression openDivRx(QStringLiteral(R"(<div\b)"), QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression closeDivRx(QStringLiteral(R"(</div\s*>)"), QRegularExpression::CaseInsensitiveOption);

        while (depth > 0 && pos < result.size()) {
            QRegularExpressionMatch openMatch = openDivRx.match(result, pos);
            QRegularExpressionMatch closeMatch = closeDivRx.match(result, pos);

            int openPos = openMatch.hasMatch() ? openMatch.capturedStart(0) : -1;
            int closePos = closeMatch.hasMatch() ? closeMatch.capturedStart(0) : -1;

            if (openPos == -1 && closePos == -1) {
                break;
            }

            if (closePos == -1 || (openPos != -1 && openPos < closePos)) {
                depth++;
                pos = openMatch.capturedEnd(0);
            } else {
                depth--;
                pos = closeMatch.capturedEnd(0);
                if (depth == 0) {
                    break;
                }
            }
        }

        int divEnd = pos;

        if (depth > 0) {
            // Malformed HTML: no matching </div>, skip this match
            searchOffset = divContentStart;
            continue;
        }

        // Decode HTML entities in the jsonKey value
        QString decodedJson = jsonKeyRaw;
        decodedJson.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
        decodedJson.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
        decodedJson.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
        decodedJson.replace(QStringLiteral("&amp;"), QStringLiteral("&"));

        // Parse JSON and extract text field
        QString transcriptText;
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(decodedJson.toUtf8(), &parseError);
        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            transcriptText = jsonObj.value(QStringLiteral("text")).toString();
        }

        // Build replacement: escaped transcript text or empty
        QString replacement;
        if (!transcriptText.trimmed().isEmpty()) {
            replacement = QStringLiteral("<p>%1</p>").arg(transcriptText.toHtmlEscaped());
        }

        result.replace(divStart, divEnd - divStart, replacement);

        // Continue searching after the replacement
        searchOffset = divStart + replacement.length();
    }

    return result;
}

QString ExportNoteWorker::getExportFileName(const QString &baseName, const QString &fileSuffix)
{
    qDebug() << "Generating unique filename for:" << baseName << fileSuffix;
    QString filePath = baseName + fileSuffix;
    int count = 1;
    while (1) {
        QFile file(filePath);
        if (!file.exists()) {
            break;
        }
        filePath = baseName + "(" + QString::number(count++) + ")" + fileSuffix;
    }
    qInfo() << "Generated unique filename:" << filePath;
    return filePath;
}
