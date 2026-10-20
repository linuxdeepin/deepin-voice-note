// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIPTAPDOCUMENTEXPORTER_H
#define TIPTAPDOCUMENTEXPORTER_H

#include <QString>
#include <QStringList>

class TiptapDocumentExporter
{
public:
    static bool isTiptapEnvelope(const QString &payload);

    // Export a Tiptap envelope to deterministic plain text. List, task and
    // blockquote structure markers are omitted so only user text remains;
    // images are emitted as explicit placeholders and voice blocks contribute
    // persisted transcript text only.
    static QString toPlainText(const QString &payload);

    // Export a Tiptap envelope to standalone HTML. Local AppData images are
    // embedded as data URLs where possible; otherwise the persisted safe src is
    // kept. Only persisted schema fields are rendered, runtime fields are
    // intentionally ignored.
    static QString toHtml(const QString &payload);

    // Return legacy voice JSON objects compatible with MetaDataParser and the
    // existing voice export worker. Runtime-only attrs are not emitted.
    static QStringList voiceJsons(const QString &payload);

    static bool hasText(const QString &payload);
    static bool hasVoice(const QString &payload);
};

#endif // TIPTAPDOCUMENTEXPORTER_H
