// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "search/searchdocumentextractor.h"
#include "search/searchindexmanager.h"
#include "search/searchtextnormalizer.h"
#include "common/vnoteitem.h"

#include <gtest/gtest.h>

namespace {

QString tiptapEnvelopeWithVoice()
{
    return QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{
            "type":"doc",
            "content":[
                {"type":"heading","attrs":{"level":1},"content":[{"type":"text","text":"项目搜索设计"}]},
                {"type":"paragraph","content":[{"type":"text","text":"Tiptap 正文内容可以被统一索引。"}]},
                {"type":"voiceBlock","attrs":{
                    "voiceId":"voice-search-1",
                    "voicePath":"voicenote/search.wav",
                    "voiceSize":1000,
                    "title":"会议录音",
                    "text":"这是一段语音转文字搜索内容",
                    "translateUnfold":false
                }}
            ]
        }
    })json");
}

VNoteItem makeNote(int id, const QString &title, const QString &metadata)
{
    VNoteItem note;
    note.noteId = id;
    note.folderId = 1;
    note.noteTitle = title;
    note.modifyTime = QDateTime::fromString(QStringLiteral("2026-08-28 10:00:00.000"),
                                            QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    note.setMetadata(metadata);
    return note;
}

} // namespace

TEST(SearchTextNormalizer, FoldsCaseWidthAndWhitespace)
{
    EXPECT_EQ(SearchTextNormalizer::normalize(QStringLiteral(" Ａbc\n\tDEF  ")),
              QStringLiteral("abc def"));
}

TEST(TiptapDocumentExtractor, ExtractsBodyAndVoiceTranscript)
{
    VNoteItem note = makeNote(10, QStringLiteral("搜索标题"), tiptapEnvelopeWithVoice());
    const SearchDocument document = SearchDocumentExtractor::extract(&note);

    QString corpus;
    for (const SearchSegment &segment : document.segments) {
        corpus += segment.text + QLatin1Char('\n');
    }

    EXPECT_TRUE(corpus.contains(QStringLiteral("项目搜索设计")));
    EXPECT_TRUE(corpus.contains(QStringLiteral("Tiptap 正文内容")));
    EXPECT_TRUE(corpus.contains(QStringLiteral("会议录音")));
    EXPECT_TRUE(corpus.contains(QStringLiteral("语音转文字搜索内容")));
}


TEST(TiptapDocumentExtractor, JoinsMarkedTextNodesWithinBlock)
{
    VNoteItem note = makeNote(14, QStringLiteral("拆分文本"), QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[
            {"type":"text","marks":[{"type":"bold"}],"text":"hello "},
            {"type":"text","marks":[{"type":"italic"}],"text":"world"}
        ]}]}
    })json"));

    SearchIndexManager index;
    index.updateNote(&note);
    QList<SearchResult> results = index.search(QStringLiteral("hello world"));
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().noteId, 14);
}

TEST(SearchIndexManager, SearchesTiptapBodyAndVoiceTranscript)
{
    VNoteItem note = makeNote(11, QStringLiteral("架构标题"), tiptapEnvelopeWithVoice());
    SearchIndexManager index;
    index.updateNote(&note);

    QList<SearchResult> bodyResults = index.search(QStringLiteral("tiptap 正文"));
    ASSERT_EQ(bodyResults.size(), 1);
    EXPECT_EQ(bodyResults.first().noteId, 11);
    EXPECT_EQ(bodyResults.first().bestField, SearchField::Body);

    QList<SearchResult> voiceResults = index.search(QStringLiteral("转文字搜索"));
    ASSERT_EQ(voiceResults.size(), 1);
    EXPECT_EQ(voiceResults.first().noteId, 11);
    EXPECT_EQ(voiceResults.first().bestField, SearchField::VoiceTranscript);
}

TEST(SearchIndexManager, UpdatesChangedNoteAndRemovesOldNgrams)
{
    VNoteItem note = makeNote(12, QStringLiteral("旧标题"), tiptapEnvelopeWithVoice());
    SearchIndexManager index;
    index.updateNote(&note);
    ASSERT_EQ(index.search(QStringLiteral("语音转文字搜索")).size(), 1);

    note.modifyTime = note.modifyTime.addSecs(1);
    note.setMetadata(QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[{"type":"text","text":"全新索引内容"}]}]}
    })json"));
    index.updateNote(&note);

    EXPECT_TRUE(index.search(QStringLiteral("语音转文字搜索")).isEmpty());
    ASSERT_EQ(index.search(QStringLiteral("全新索引")).size(), 1);
}

TEST(VNoteItemSearchCompatibility, DelegatesToUnifiedExtractor)
{
    VNoteItem note = makeNote(13, QStringLiteral("兼容入口"), tiptapEnvelopeWithVoice());
    EXPECT_TRUE(note.search(QStringLiteral("语音转文字搜索")));
    EXPECT_TRUE(note.search(QStringLiteral("tiptap 正文")));
    EXPECT_FALSE(note.search(QStringLiteral("不存在的关键字")));
}
