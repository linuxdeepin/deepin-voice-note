// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchindexmanager.h"

#include "searchdocumentextractor.h"
#include "searchtextnormalizer.h"
#include "utils.h"
#include "vnoteitem.h"

#include <QCollator>
#include <QReadLocker>
#include <QRegularExpression>
#include <QTextDocument>
#include <QWriteLocker>
#include <algorithm>

void SearchIndexManager::clear()
{
    QWriteLocker locker(&m_lock);
    m_entries.clear();
    m_ngramIndex.clear();
}

void SearchIndexManager::refreshFromNotes(VNOTE_ALL_NOTES_MAP *noteAll)
{
    if (!noteAll) {
        clear();
        return;
    }

    QSet<int> aliveNoteIds;
    noteAll->lock.lockForRead();
    for (auto folderIt = noteAll->notes.constBegin(); folderIt != noteAll->notes.constEnd(); ++folderIt) {
        VNOTE_ITEMS_MAP *folderNotes = folderIt.value();
        if (!folderNotes) {
            continue;
        }
        folderNotes->lock.lockForRead();
        for (auto noteIt = folderNotes->folderNotes.constBegin(); noteIt != folderNotes->folderNotes.constEnd(); ++noteIt) {
            const VNoteItem *note = noteIt.value();
            if (!note) {
                continue;
            }
            aliveNoteIds.insert(note->noteId);
            updateNote(note);
        }
        folderNotes->lock.unlock();
    }
    noteAll->lock.unlock();

    QWriteLocker locker(&m_lock);
    const QList<int> indexedIds = m_entries.keys();
    for (int noteId : indexedIds) {
        if (!aliveNoteIds.contains(noteId)) {
            removeEntryUnlocked(noteId);
        }
    }
}

void SearchIndexManager::updateNote(const VNoteItem *note)
{
    if (!note) {
        return;
    }

    const quint64 fingerprint = fingerprintForNote(note);
    {
        QReadLocker locker(&m_lock);
        const auto it = m_entries.constFind(note->noteId);
        if (it != m_entries.constEnd() && it->fingerprint == fingerprint) {
            return;
        }
    }

    SearchIndexEntry entry;
    entry.document = SearchDocumentExtractor::extract(note);
    entry.fingerprint = fingerprint;
    entry.normalizedTitle = SearchTextNormalizer::normalize(entry.document.titlePlain);
    entry.normalizedCorpus = SearchTextNormalizer::normalize(corpusForDocument(entry.document));
    const qint64 gramTextLength = static_cast<qint64>(entry.normalizedTitle.size())
        + 1 + static_cast<qint64>(entry.normalizedCorpus.size());
    entry.gramIndexed = gramTextLength <= MaxGramIndexedTextLength;

    QWriteLocker locker(&m_lock);
    upsertEntryUnlocked(std::move(entry));
}

void SearchIndexManager::removeNote(int noteId)
{
    QWriteLocker locker(&m_lock);
    removeEntryUnlocked(noteId);
}

QList<SearchResult> SearchIndexManager::search(const QString &query) const
{
    const QString rawQuery = query.trimmed();
    const QString normalizedQuery = SearchTextNormalizer::normalize(rawQuery);
    if (normalizedQuery.isEmpty()) {
        return {};
    }

    QReadLocker locker(&m_lock);
    QSet<int> candidates;
    const QSet<quint64> queryGrams = gramsForText(normalizedQuery);
    if (queryGrams.isEmpty()) {
        for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
            candidates.insert(it.key());
        }
    } else {
        bool first = true;
        for (quint64 gram : queryGrams) {
            QSet<int> ids;
            const QList<int> noteIds = m_ngramIndex.values(gram);
            for (int noteId : noteIds) {
                ids.insert(noteId);
            }
            if (first) {
                candidates = ids;
                first = false;
            } else {
                candidates.intersect(ids);
            }
            if (candidates.isEmpty()) {
                break;
            }
        }

        // 超长笔记不进入 n-gram 索引，但仍然参与最终 contains 校验。
        for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
            if (!it->gramIndexed) {
                candidates.insert(it.key());
            }
        }
    }

    QList<SearchResult> results;
    results.reserve(candidates.size());
    for (int noteId : std::as_const(candidates)) {
        const auto it = m_entries.constFind(noteId);
        if (it == m_entries.constEnd()) {
            continue;
        }
        if (!it->normalizedTitle.contains(normalizedQuery)
            && !it->normalizedCorpus.contains(normalizedQuery)) {
            continue;
        }
        results.append(makeResult(*it, normalizedQuery, rawQuery));
    }

    std::sort(results.begin(), results.end(), [](const SearchResult &left, const SearchResult &right) {
        if (left.score != right.score) {
            return left.score > right.score;
        }
        if (left.isTop != right.isTop) {
            return left.isTop;
        }
        if (left.modifyTime != right.modifyTime) {
            return left.modifyTime > right.modifyTime;
        }
        return left.noteId > right.noteId;
    });
    return results;
}

int SearchIndexManager::size() const
{
    QReadLocker locker(&m_lock);
    return m_entries.size();
}

quint64 SearchIndexManager::fingerprintForNote(const VNoteItem *note)
{
    if (!note) {
        return 0;
    }

    // 只保留 64 位摘要，避免把整篇正文作为 fingerprint 常驻内存。
    auto mix = [](quint64 hash, quint64 value) {
        return hash ^ (value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2));
    };

    quint64 hash = 0xcbf29ce484222325ULL;
    hash = mix(hash, static_cast<quint64>(note->noteId));
    hash = mix(hash, static_cast<quint64>(note->folderId));
    hash = mix(hash, static_cast<quint64>(note->isTop));
    hash = mix(hash, static_cast<quint64>(note->modifyTime.toMSecsSinceEpoch()));
    hash = mix(hash, static_cast<quint64>(qHash(note->noteTitle)));
    hash = mix(hash, static_cast<quint64>(qHash(note->htmlCode)));
    hash = mix(hash, static_cast<quint64>(qHash(note->metaDataConstRef().toString())));
    return hash;
}

QString SearchIndexManager::corpusForDocument(const SearchDocument &document)
{
    QString corpus;
    for (const SearchSegment &segment : document.segments) {
        if (segment.field == SearchField::Title) {
            continue;
        }
        if (!corpus.isEmpty()) {
            corpus.append(QLatin1Char('\n'));
        }
        corpus.append(segment.text);
    }
    return corpus;
}

QSet<quint64> SearchIndexManager::gramsForText(const QString &text)
{
    QSet<quint64> grams;
    if (text.size() < NGramSize) {
        return grams;
    }

    for (qsizetype i = 0; i <= text.size() - NGramSize; ++i) {
        const QChar first = text.at(i);
        const QChar second = text.at(i + 1);
        const QChar third = text.at(i + 2);
        if (first.isSpace() && second.isSpace() && third.isSpace()) {
            continue;
        }

        const quint64 gram = (static_cast<quint64>(first.unicode()) << 32)
            | (static_cast<quint64>(second.unicode()) << 16)
            | static_cast<quint64>(third.unicode());
        grams.insert(gram);
    }
    return grams;
}

int SearchIndexManager::fieldWeight(SearchField field)
{
    switch (field) {
    case SearchField::Title:
        return 100;
    case SearchField::Body:
        return 45;
    case SearchField::VoiceTranscript:
        return 40;
    case SearchField::VoiceTitle:
        return 35;
    case SearchField::ImageAlt:
        return 20;
    }
    return 10;
}

QString SearchIndexManager::highlightedHtml(const QString &text, const QString &query)
{
    return Utils::createRichText(text, query);
}

QString SearchIndexManager::snippetHtml(const QString &text, const QString &query)
{
    if (text.isEmpty()) {
        return QString();
    }
    const int match = text.indexOf(query, 0, Qt::CaseInsensitive);
    if (match < 0) {
        return QString();
    }

    constexpr int Radius = 36;
    const int start = qMax(0, match - Radius);
    const int end = qMin(text.size(), match + query.size() + Radius);
    QString snippet = text.mid(start, end - start).simplified();
    QString prefix = start > 0 ? QStringLiteral("...") : QString();
    QString suffix = end < text.size() ? QStringLiteral("...") : QString();
    return prefix + highlightedHtml(snippet, query) + suffix;
}

SearchResult SearchIndexManager::makeResult(const SearchIndexEntry &entry,
                                            const QString &normalizedQuery,
                                            const QString &rawQuery)
{
    SearchResult result;
    result.noteId = entry.document.noteId;
    result.folderId = entry.document.folderId;
    result.isTop = entry.document.isTop;
    result.modifyTime = entry.document.modifyTime;
    result.titlePlain = entry.document.titlePlain;
    result.highlightedTitle = highlightedHtml(entry.document.titlePlain, rawQuery);

    int bestScore = 0;
    SearchMatch bestMatch;
    for (const SearchSegment &segment : entry.document.segments) {
        const QString normalizedText = SearchTextNormalizer::normalize(segment.text);
        if (!normalizedText.contains(normalizedQuery)) {
            continue;
        }

        SearchMatch match;
        match.field = segment.field;
        match.text = segment.text;
        match.locatorType = segment.locatorType;
        match.locator = segment.locator;
        match.position = normalizedText.indexOf(normalizedQuery);
        result.matches.append(match);

        int score = fieldWeight(segment.field);
        if (match.position == 0) {
            score += 8;
        }
        if (segment.field == SearchField::Title
            && SearchTextNormalizer::normalize(segment.text) == normalizedQuery) {
            score += 20;
        }
        if (score > bestScore) {
            bestScore = score;
            bestMatch = match;
        }
    }

    result.score = bestScore;
    result.bestField = bestMatch.field;
    if (bestMatch.field != SearchField::Title) {
        result.snippet = snippetHtml(bestMatch.text, rawQuery);
    }
    return result;
}

void SearchIndexManager::upsertEntryUnlocked(SearchIndexEntry &&entry)
{
    removeEntryUnlocked(entry.document.noteId);
    const int noteId = entry.document.noteId;
    if (entry.gramIndexed) {
        const QSet<quint64> grams = gramsForText(entry.normalizedTitle + QLatin1Char(' ') + entry.normalizedCorpus);
        for (quint64 gram : grams) {
            m_ngramIndex.insert(gram, noteId);
        }
    }
    m_entries.insert(noteId, std::move(entry));
}

void SearchIndexManager::removeEntryUnlocked(int noteId)
{
    const auto it = m_entries.find(noteId);
    if (it == m_entries.end()) {
        return;
    }
    if (it->gramIndexed) {
        const QSet<quint64> grams = gramsForText(it->normalizedTitle + QLatin1Char(' ') + it->normalizedCorpus);
        for (quint64 gram : grams) {
            m_ngramIndex.remove(gram, noteId);
        }
    }
    m_entries.erase(it);
}
