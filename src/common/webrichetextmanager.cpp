// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "webrichetextmanager.h"
#include "ddlog.h"
#include "jscontent.h"
#include "db/vnoteitemoper.h"
#include "metadataparser.h"
#include "migrationhtmlconverter.h"
#include "migrationjsonbuilder.h"
#include "migrationnotedataconverter.h"
#include "common/utils.h"
#include "handler/voice_to_text_task_manager.h"
#include "tiptapchannelbridge.h"
#include "legacyformatdetector.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QUuid>

namespace {

QString emptyTiptapEnvelopeJson()
{
    return MigrationJsonBuilder::toCompactJson(MigrationJsonBuilder::makeEnvelope());
}

QString legacyHtmlFromPayload(const QString &payload, const QString &parsedHtml)
{
    if (!parsedHtml.isEmpty()) {
        return parsedHtml;
    }

    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8());
    if (document.isObject()) {
        return document.object().value(QStringLiteral("htmlCode")).toString();
    }
    return payload;
}

QString envelopeForNote(const VNoteItem &data)
{
    const QString payload = data.metaDataConstRef().toString();
    const LegacyFormat format = LegacyFormatDetector::detect(payload);

    switch (format) {
    case LegacyFormat::TiptapEnvelope:
        return payload;
    case LegacyFormat::LegacyHtmlCode: {
        const MigrationHtmlConversionResult converted =
            MigrationHtmlConverter::convert(legacyHtmlFromPayload(payload, data.htmlCode));
        if (converted.ok()) {
            return MigrationJsonBuilder::toCompactJson(converted.envelope);
        }
        qCWarning(voiceNoteLog) << "Failed to convert legacy HTML note to Tiptap envelope:" << converted.errors.size();
        break;
    }
    case LegacyFormat::LegacyNoteDatas: {
        const MigrationNoteDataConversionResult converted =
            MigrationNoteDataConverter::convertBlocks(payload);
        if (converted.ok()) {
            return MigrationJsonBuilder::toCompactJson(converted.envelope);
        }
        qCWarning(voiceNoteLog) << "Failed to convert legacy note data to Tiptap envelope:" << converted.errors.size();
        break;
    }
    case LegacyFormat::ProseMirrorDoc: {
        const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8());
        if (document.isObject()) {
            return MigrationJsonBuilder::toCompactJson(
                MigrationJsonBuilder::makeEnvelope(document.object()));
        }
        break;
    }
    case LegacyFormat::PlainText: {
        QJsonArray paragraphContent;
        if (!payload.isEmpty()) {
            paragraphContent.append(MigrationJsonBuilder::makeText(payload));
        }
        return MigrationJsonBuilder::toCompactJson(
            MigrationJsonBuilder::makeEnvelope(
                MigrationJsonBuilder::makeDoc(
                    QJsonArray{MigrationJsonBuilder::makeParagraph(paragraphContent)})));
    }
    case LegacyFormat::Invalid:
        break;
    }

    // A note switch must never leave the previous note's document visible.
    return emptyTiptapEnvelopeJson();
}

} // namespace


WebRichTextManager::WebRichTextManager(QObject *parent)
: QObject(parent)
{
    qCInfo(voiceNoteLog) << "WebRichTextManager constructor called";
    initUpdateTimer();
    initConnect();
    qCInfo(voiceNoteLog) << "WebRichTextManager constructor finished";
}

void WebRichTextManager::initData(VNoteItem *data, const QString reg, bool focus)
{
    qCInfo(voiceNoteLog) << "Initializing data for note ID:" << (data ? data->noteId : -1);
    //重置鼠标点击位置
    m_mouseClickPos = QPoint(-1, -1);
    m_setFocus = focus;
    // 若数据为空，清空页面并返回，避免后续异步回调空指针
    if (!data) {
        qCWarning(voiceNoteLog) << "initData called with null data, clearing JS content";
        m_noteData = nullptr;
        m_textChange = false;
        m_textChangeNoteId = -1;
        m_updateInProgress = false;
        m_updateRequestNoteId = -1;
        m_updateRequestSerial = 0;
        if (m_updateTimer) m_updateTimer->stop();
        clearJSContent();
        return;
    }
    // 保存当前数据供 onLoadFinsh 以及异步回调使用
    m_noteData = data;
    //富文本设置异步操作，解决笔记列表不实时刷新
    QTimer::singleShot(50, this, [this, reg] {
        // 使用成员 m_noteData 避免捕获过期指针
        if (!m_noteData) {
            qCWarning(voiceNoteLog) << "Deferred setData skipped due to null m_noteData";
            return;
        }
        setData(m_noteData, reg);
    });
    qCInfo(voiceNoteLog) << "Data initialization finished";
}

void WebRichTextManager::initConnect()
{
    qCInfo(voiceNoteLog) << "Initializing connections";
    connect(JsContent::instance(), &JsContent::textChange, this, [=](){
        m_textChange = true;
        m_textChangeNoteId = m_noteData ? m_noteData->noteId : -1;
        ++m_textChangeSerial;
        emit noteTextChanged();
    });

    connect(JsContent::instance(), &JsContent::scrollTopChange, this, [=](bool isTop){
        emit scrollChange(isTop);
    });
    connect(JsContent::instance(), &JsContent::setDataFinsh, this, &WebRichTextManager::onSetDataFinsh);

    connect(TiptapChannelBridge::instance(), &TiptapChannelBridge::contentChanged, this, [=](){
        m_textChange = true;
        m_textChangeNoteId = m_noteData ? m_noteData->noteId : -1;
        ++m_textChangeSerial;
        emit noteTextChanged();
    });
    connect(TiptapChannelBridge::instance(), &TiptapChannelBridge::contentSaved,
            this, &WebRichTextManager::onTiptapContentSaved);

    qCInfo(voiceNoteLog) << "Connections initialized";
}

void WebRichTextManager::setData(VNoteItem *data, const QString reg)
{
    qCDebug(voiceNoteLog) << "Setting rich text data";
    if (!data) {
        qCWarning(voiceNoteLog) << "setData called with null data, skip";
        return;
    }
    m_noteData = data;
    m_updateTimer->stop();

    if (TiptapChannelBridge::instance()->tiptapEnabled()) {
        TiptapChannelBridge::instance()->loadEnvelope(envelopeForNote(*data));
        m_updateTimer->start();
        emit updateSearch();
        return;
    }

    //设置富文本内容
    if (data->htmlCode.isEmpty()) {
        qCDebug(voiceNoteLog) << "Initializing with metadata";
        emit JsContent::instance()->callJsInitData(data->metaDataRef().toString());
    } else {
        qCDebug(voiceNoteLog) << "Setting HTML content";
        emit JsContent::instance()->callJsSetHtml(data->htmlCode);
    }
    m_updateTimer->start();
    emit updateSearch();

    // 检查该笔记是否有转换任务，恢复相应状态
    int noteId = data->noteId;
    QTimer::singleShot(500, this, [noteId]() {
        auto tasks = VoiceToTextTaskManager::instance()->getTasksForNote(noteId);
        for (const auto &task : tasks) {
            if (task.status == VoiceToTextTask::Converting) {
                // 任务还在进行中，显示转换中状态（使用 voiceId 定位）
                qCInfo(voiceNoteLog) << "Restoring voice-to-text converting status for voiceId:" << task.voiceId;
                emit JsContent::instance()->callJsSetVoiceTextByPath(
                    task.voiceId, QString(), JsContent::AsrFlag::Start);
            } else if (task.status == VoiceToTextTask::Completed && !task.resultText.isEmpty()) {
                // 任务已完成且有结果，确保结果显示
                qCInfo(voiceNoteLog) << "Task completed, ensuring result is displayed for voiceId:" << task.voiceId;
                emit JsContent::instance()->callJsSetVoiceTextByPath(
                    task.voiceId, task.resultText, JsContent::AsrFlag::End);
                // 移除已处理的任务
                VoiceToTextTaskManager::instance()->removeTask(task.voiceId);
            } else if (task.status == VoiceToTextTask::Failed) {
                // 任务失败，移除转换中状态
                qCInfo(voiceNoteLog) << "Task failed, removing converting status for voiceId:" << task.voiceId;
                emit JsContent::instance()->callJsSetVoiceTextByPath(
                    task.voiceId, QString(), JsContent::AsrFlag::End);
                // 移除已处理的任务
                VoiceToTextTaskManager::instance()->removeTask(task.voiceId);
            }
        }
    });

    qCInfo(voiceNoteLog) << "Rich text data setting finished";
}

void WebRichTextManager::initUpdateTimer()
{
    qCInfo(voiceNoteLog) << "Initializing update timer";
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &WebRichTextManager::updateNote);
    qCInfo(voiceNoteLog) << "Update timer initialized";
}

void WebRichTextManager::updateNote()
{
    // qCInfo(voiceNoteLog) << "Updating note, text change flag:" << m_textChange;
    if (m_textChange) {
        if (m_updateInProgress) {
            return;
        }
        m_updateInProgress = true;
        m_updateRequestNoteId = m_textChangeNoteId;
        m_updateRequestSerial = m_textChangeSerial;
        emit needUpdateNote(m_updateRequestNoteId);
    }
    // qCInfo(voiceNoteLog) << "Note update finished";
}

bool WebRichTextManager::hasPendingTextChange() const
{
    return m_textChange;
}

int WebRichTextManager::pendingTextChangeNoteId() const
{
    return m_textChangeNoteId;
}

int WebRichTextManager::currentNoteId() const
{
    return m_noteData ? m_noteData->noteId : -1;
}

void WebRichTextManager::requestUpdateNoteNow()
{
    updateNote();
}

void WebRichTextManager::onUpdateNoteWithResult(VNoteItem *data, const QString &result)
{
    qCDebug(voiceNoteLog) << "Updating note with result";
    if (!data) {
        qCWarning(voiceNoteLog) << "onUpdateNoteWithResult called with null data, skip";
        m_updateInProgress = false;
        m_updateRequestNoteId = -1;
        m_updateRequestSerial = 0;
        emit finishedUpdateNote();
        return;
    }
    data->htmlCode = result;
    VNoteItemOper noteOps(data);
    const bool updateOk = noteOps.updateNote();
    if (!updateOk) {
        qCWarning(voiceNoteLog) << "Failed to save note";
    } else {
        qCDebug(voiceNoteLog) << "Note saved successfully";
    }
    if (updateOk && data->noteId == m_textChangeNoteId && m_updateRequestSerial == m_textChangeSerial) {
        m_textChange = false;
        m_textChangeNoteId = -1;
    }
    m_updateInProgress = false;
    m_updateRequestNoteId = -1;
    m_updateRequestSerial = 0;
    emit finishedUpdateNote();
    qCInfo(voiceNoteLog) << "Note update with result finished";
}

void WebRichTextManager::onTiptapContentSaved(const QString &envelopeJson)
{
    qCDebug(voiceNoteLog) << "Saving Tiptap envelope content";
    if (!m_noteData) {
        qCWarning(voiceNoteLog) << "onTiptapContentSaved: null note data, skip";
        m_updateInProgress = false;
        m_updateRequestNoteId = -1;
        m_updateRequestSerial = 0;
        emit finishedUpdateNote();
        return;
    }
    bool updateOk = VNoteItemOper(m_noteData).updateNoteTiptapEnvelope(envelopeJson);
    if (!updateOk) {
        qCWarning(voiceNoteLog) << "Failed to save Tiptap note";
    } else {
        qCDebug(voiceNoteLog) << "Tiptap note saved successfully";
    }
    if (updateOk && m_noteData->noteId == m_textChangeNoteId && m_updateRequestSerial == m_textChangeSerial) {
        m_textChange = false;
        m_textChangeNoteId = -1;
    }
    m_updateInProgress = false;
    m_updateRequestNoteId = -1;
    m_updateRequestSerial = 0;
    emit finishedUpdateNote();
    qCInfo(voiceNoteLog) << "Tiptap content saved finished";
}

void WebRichTextManager::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    qCDebug(voiceNoteLog) << "Inserting voice item, path:" << voicePath << "size:" << voiceSize;
    VNVoiceBlock data;
    data.ptrVoice->voiceSize = voiceSize;
    // 存储相对路径：AppData/voicenote/ -> voicenote/...
    data.ptrVoice->voicePath = Utils::makeVoiceRelative(voicePath);
    data.ptrVoice->createTime = QDateTime::currentDateTime();
    data.ptrVoice->voiceTitle = data.ptrVoice->createTime.toString("yyyyMMdd hh.mm.ss");
    // 为新录音生成唯一标识（UUID）
    data.ptrVoice->voiceId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    MetaDataParser parse;
    QVariant value;
    parse.makeMetaData(&data, value);

    const QString voiceInfoJson = value.toString();
    if (TiptapChannelBridge::instance()->tiptapEnabled()) {
        TiptapChannelBridge::instance()->sendInsertVoiceBlock(voiceInfoJson);
        qCDebug(voiceNoteLog) << "Tiptap voice block insert requested successfully, voiceId:" << data.ptrVoice->voiceId;
        return;
    }

    emit JsContent::instance()->callJsInsertVoice(voiceInfoJson);
    qCDebug(voiceNoteLog) << "Voice item inserted successfully, voiceId:" << data.ptrVoice->voiceId;
}

/**
 * @brief onLoadFinsh
 * 与web端通信建立完成
 */
void WebRichTextManager::onLoadFinsh()
{
    qCDebug(voiceNoteLog) << "Web communication established";
    //再次设置笔记内容
    if (m_noteData && !m_loadFinshSign) {
        if (TiptapChannelBridge::instance()->tiptapEnabled()) {
            qCDebug(voiceNoteLog) << "Reloading Tiptap envelope after load";
            TiptapChannelBridge::instance()->loadEnvelope(envelopeForNote(*m_noteData));
        } else if (m_noteData->htmlCode.isEmpty()) {
            qCDebug(voiceNoteLog) << "Initializing with metadata after load";
            emit JsContent::instance()->callJsInitData(m_noteData->metaDataRef().toString());
        } else {
            qCDebug(voiceNoteLog) << "Setting HTML content after load";
            emit JsContent::instance()->callJsSetHtml(m_noteData->htmlCode);
        }
    }
    m_loadFinshSign = true;
    qCInfo(voiceNoteLog) << "Web communication establishment handling finished";
}

void WebRichTextManager::clearJSContent()
{
    qCDebug(voiceNoteLog) << "Clearing JS content";
    if (TiptapChannelBridge::instance()->tiptapEnabled()) {
        TiptapChannelBridge::instance()->loadEnvelope(emptyTiptapEnvelopeJson());
    }
    emit JsContent::instance()->callJsSetHtml("");

    // 开启100ms事件循环，保证js页面内容被刷新
    QEventLoop eveLoop;
    QTimer::singleShot(100, &eveLoop, SLOT(quit()));
    eveLoop.exec();
    qCDebug(voiceNoteLog) << "JS content cleared";
}

void WebRichTextManager::onSetDataFinsh()
{
    qCInfo(voiceNoteLog) << "Set data finished";
    if (!m_setFocus) {
        return;
    }

    m_setFocus = false;
    emit JsContent::instance()->callJsFocusEditor();
}
