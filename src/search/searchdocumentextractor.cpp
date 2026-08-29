// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchdocumentextractor.h"

#include "legacyformatdetector.h"
#include "vnoteitem.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextDocument>
#include <QSet>

namespace {

SearchDocument baseDocument(const VNoteItem *note)
{
    SearchDocument document;
    if (!note) {
        return document;
    }
    document.noteId = note->noteId;
    document.folderId = static_cast<int>(note->folderId);
    document.isTop = note->isTop != 0;
    document.modifyTime = note->modifyTime;
    document.titlePlain = note->noteTitle;
    document.segments.append({SearchField::Title, note->noteTitle,
                              QStringLiteral("title"), QString()});
    return document;
}

void appendSegment(SearchDocument *document, SearchField field, const QString &text,
                   const QString &locatorType = QString(), const QString &locator = QString())
{
    if (!document || text.trimmed().isEmpty()) {
        return;
    }
    document->segments.append({field, text, locatorType, locator});
}

QString plainTextFromHtml(const QString &html)
{
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText();
}

bool isTiptapTextBlock(const QString &type)
{
    static const QSet<QString> blockTypes = {
        QStringLiteral("paragraph"),
        QStringLiteral("heading"),
        QStringLiteral("listItem"),
        QStringLiteral("taskItem"),
        QStringLiteral("blockquote"),
    };
    return blockTypes.contains(type);
}

QString collectTiptapInlineText(const QJsonObject &node, SearchDocument *document)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    const QJsonObject attrs = node.value(QStringLiteral("attrs")).toObject();

    if (type == QLatin1String("text")) {
        return node.value(QStringLiteral("text")).toString();
    }

    if (type == QLatin1String("hardBreak")) {
        return QStringLiteral("\n");
    }

    if (type == QLatin1String("voiceBlock")) {
        const QString voiceId = attrs.value(QStringLiteral("voiceId")).toString();
        appendSegment(document, SearchField::VoiceTitle,
                      attrs.value(QStringLiteral("title")).toString(),
                      QStringLiteral("voice"), voiceId);
        appendSegment(document, SearchField::VoiceTranscript,
                      attrs.value(QStringLiteral("text")).toString(),
                      QStringLiteral("voice"), voiceId);
        return QString();
    }

    if (type == QLatin1String("image")) {
        const QString locator = attrs.value(QStringLiteral("relPath")).toString();
        appendSegment(document, SearchField::ImageAlt,
                      attrs.value(QStringLiteral("alt")).toString(),
                      QStringLiteral("image"), locator);
        appendSegment(document, SearchField::ImageAlt,
                      attrs.value(QStringLiteral("title")).toString(),
                      QStringLiteral("image"), locator);
        return QString();
    }

    QString text;
    const QJsonArray children = node.value(QStringLiteral("content")).toArray();
    for (const QJsonValue &child : children) {
        if (!child.isObject()) {
            continue;
        }
        const QString childText = collectTiptapInlineText(child.toObject(), document);
        if (!text.isEmpty() && !childText.isEmpty()
            && !text.endsWith(QLatin1Char('\n')) && !childText.startsWith(QLatin1Char('\n'))) {
            // Text nodes that were split only because of marks must remain adjacent.
            // Container boundaries keep a light separator to avoid accidental word joins.
            const QString childType = child.toObject().value(QStringLiteral("type")).toString();
            if (childType != QLatin1String("text")) {
                text.append(QLatin1Char(' '));
            }
        }
        text.append(childText);
    }
    return text;
}

void appendTiptapBlocks(const QJsonObject &node, SearchDocument *document)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("voiceBlock") || type == QLatin1String("image")) {
        collectTiptapInlineText(node, document);
        return;
    }

    if (isTiptapTextBlock(type)) {
        appendSegment(document, SearchField::Body, collectTiptapInlineText(node, document),
                      QStringLiteral("pm-block"));
        return;
    }

    const QJsonArray children = node.value(QStringLiteral("content")).toArray();
    for (const QJsonValue &child : children) {
        if (child.isObject()) {
            appendTiptapBlocks(child.toObject(), document);
        }
    }
}


class TiptapDocumentExtractor final : public ISearchDocumentExtractor
{
public:
    bool canExtract(const VNoteItem *note) const override
    {
        return note && LegacyFormatDetector::detect(note->metaDataConstRef().toString())
            == LegacyFormat::TiptapEnvelope;
    }

    SearchDocument extract(const VNoteItem *note) const override
    {
        SearchDocument document = baseDocument(note);
        QJsonParseError parseError;
        const QJsonDocument json = QJsonDocument::fromJson(
            note->metaDataConstRef().toString().toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
            return document;
        }

        const QJsonObject root = json.object();
        appendTiptapBlocks(root.value(QStringLiteral("content")).toObject(), &document);
        return document;
    }
};

class LegacyHtmlExtractor final : public ISearchDocumentExtractor
{
public:
    bool canExtract(const VNoteItem *note) const override
    {
        if (!note) {
            return false;
        }
        if (!note->htmlCode.isEmpty()) {
            return true;
        }
        const LegacyFormat format = LegacyFormatDetector::detect(note->metaDataConstRef().toString());
        return format == LegacyFormat::LegacyHtmlCode;
    }

    SearchDocument extract(const VNoteItem *note) const override
    {
        SearchDocument document = baseDocument(note);
        QString html = note->htmlCode;
        if (html.isEmpty()) {
            const QString meta = note->metaDataConstRef().toString();
            QJsonDocument json = QJsonDocument::fromJson(meta.toUtf8());
            if (json.isObject()) {
                html = json.object().value(QStringLiteral("htmlCode")).toString();
            } else {
                html = meta;
            }
        }
        appendSegment(&document, SearchField::Body, plainTextFromHtml(html),
                      QStringLiteral("legacy-html"));
        return document;
    }
};

class LegacyNoteDatasExtractor final : public ISearchDocumentExtractor
{
public:
    bool canExtract(const VNoteItem *note) const override
    {
        if (!note) {
            return false;
        }
        const LegacyFormat format = LegacyFormatDetector::detect(note->metaDataConstRef().toString());
        return format == LegacyFormat::LegacyNoteDatas || !note->datas.dataConstRef().isEmpty();
    }

    SearchDocument extract(const VNoteItem *note) const override
    {
        SearchDocument document = baseDocument(note);
        for (const VNoteBlock *block : note->datas.dataConstRef()) {
            if (!block) {
                continue;
            }
            if (block->getType() == VNoteBlock::Voice) {
                appendSegment(&document, SearchField::VoiceTranscript, block->blockText,
                              QStringLiteral("legacy-voice"), block->ptrVoice->voiceId);
                appendSegment(&document, SearchField::VoiceTitle, block->ptrVoice->voiceTitle,
                              QStringLiteral("legacy-voice"), block->ptrVoice->voiceId);
            } else {
                appendSegment(&document, SearchField::Body, block->blockText,
                              QStringLiteral("legacy-block"));
            }
        }
        return document;
    }
};

class PlainTextExtractor final : public ISearchDocumentExtractor
{
public:
    bool canExtract(const VNoteItem *note) const override
    {
        return note != nullptr;
    }

    SearchDocument extract(const VNoteItem *note) const override
    {
        SearchDocument document = baseDocument(note);
        const QString meta = note->metaDataConstRef().toString();
        if (LegacyFormatDetector::detect(meta) == LegacyFormat::PlainText) {
            appendSegment(&document, SearchField::Body, meta, QStringLiteral("plain-text"));
        }
        return document;
    }
};

} // namespace

SearchDocument SearchDocumentExtractor::extract(const VNoteItem *note)
{
    static const TiptapDocumentExtractor tiptapExtractor;
    static const LegacyHtmlExtractor htmlExtractor;
    static const LegacyNoteDatasExtractor legacyExtractor;
    static const PlainTextExtractor plainTextExtractor;
    static const ISearchDocumentExtractor *extractors[] = {
        &tiptapExtractor,
        &htmlExtractor,
        &legacyExtractor,
        &plainTextExtractor,
    };

    for (const ISearchDocumentExtractor *extractor : extractors) {
        if (extractor->canExtract(note)) {
            return extractor->extract(note);
        }
    }
    return baseDocument(note);
}
