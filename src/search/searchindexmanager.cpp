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

    const QString fingerprint = fingerprintForNote(note);
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
    entry.grams = gramsForText(entry.normalizedTitle + QLatin1Char(' ') + entry.normalizedCorpus);

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
    const QSet<QString> queryGrams = gramsForText(normalizedQuery);
    if (queryGrams.isEmpty()) {
        for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
            candidates.insert(it.key());
        }
    } else {
        bool first = true;
        for (const QString &gram : queryGrams) {
            const QSet<int> ids = m_ngramIndex.value(gram);
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

QString SearchIndexManager::fingerprintForNote(const VNoteItem *note)
{
    if (!note) {
        return QString();
    }
    return QString::number(note->noteId)
        + QLatin1Char('\x1f') + QString::number(note->folderId)
        + QLatin1Char('\x1f') + QString::number(note->isTop)
        + QLatin1Char('\x1f') + QString::number(note->modifyTime.toMSecsSinceEpoch())
        + QLatin1Char('\x1f') + note->noteTitle
        + QLatin1Char('\x1f') + note->htmlCode
        + QLatin1Char('\x1f') + note->metaDataConstRef().toString();
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

QSet<QString> SearchIndexManager::gramsForText(const QString &text)
{
    QSet<QString> grams;
    if (text.size() < NGramSize) {
        return grams;
    }
    for (int i = 0; i <= text.size() - NGramSize; ++i) {
        const QString gram = text.mid(i, NGramSize);
        if (!gram.trimmed().isEmpty()) {
            grams.insert(gram);
        }
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
    for (const QString &gram : std::as_const(entry.grams)) {
        m_ngramIndex[gram].insert(noteId);
    }
    m_entries.insert(noteId, std::move(entry));
}

void SearchIndexManager::removeEntryUnlocked(int noteId)
{
    const auto it = m_entries.find(noteId);
    if (it == m_entries.end()) {
        return;
    }
    for (const QString &gram : std::as_const(it->grams)) {
        auto gramIt = m_ngramIndex.find(gram);
        if (gramIt == m_ngramIndex.end()) {
            continue;
        }
        gramIt->remove(noteId);
        if (gramIt->isEmpty()) {
            m_ngramIndex.erase(gramIt);
        }
    }
    m_entries.erase(it);
}
