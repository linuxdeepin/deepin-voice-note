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
                    "title":"20260920 13.42.36",
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

TEST(TiptapDocumentExtractor, ExtractsBodyButExcludesVoiceBlock)
{
    VNoteItem note = makeNote(10, QStringLiteral("搜索标题"), tiptapEnvelopeWithVoice());
    const SearchDocument document = SearchDocumentExtractor::extract(&note);

    QString corpus;
    for (const SearchSegment &segment : document.segments) {
        corpus += segment.text + QLatin1Char('\n');
    }

    EXPECT_TRUE(corpus.contains(QStringLiteral("项目搜索设计")));
    EXPECT_TRUE(corpus.contains(QStringLiteral("Tiptap 正文内容")));
    EXPECT_FALSE(corpus.contains(QStringLiteral("20260920")));
    EXPECT_FALSE(corpus.contains(QStringLiteral("语音转文字搜索内容")));
}


TEST(LegacyHtmlExtractor, ExcludesVoiceWidgetDisplayText)
{
    VNoteItem note = makeNote(15, QStringLiteral("旧版语音"), QString());
    note.htmlCode = QStringLiteral(
        R"html(<p>普通正文关键词</p><div class="li voiceBox"><div class="voiceInfoBox"><div class="voicePlayback"><div class="title">20260920 13.42.36</div><div class="timeTotal">00:02</div></div><div class="translateText">语音转写关键词</div></div></div><p>正文结尾</p>)html");

    SearchIndexManager index;
    index.updateNote(&note);
    ASSERT_EQ(index.search(QStringLiteral("普通正文关键词")).size(), 1);
    EXPECT_TRUE(index.search(QStringLiteral("20260920")).isEmpty());
    EXPECT_TRUE(index.search(QStringLiteral("语音转写关键词")).isEmpty());
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

TEST(SearchIndexManager, SearchesTiptapBodyButExcludesVoiceBlock)
{
    VNoteItem note = makeNote(11, QStringLiteral("架构标题"), tiptapEnvelopeWithVoice());
    SearchIndexManager index;
    index.updateNote(&note);

    QList<SearchResult> bodyResults = index.search(QStringLiteral("tiptap 正文"));
    ASSERT_EQ(bodyResults.size(), 1);
    EXPECT_EQ(bodyResults.first().noteId, 11);
    EXPECT_EQ(bodyResults.first().bestField, SearchField::Body);

    EXPECT_TRUE(index.search(QStringLiteral("转文字搜索")).isEmpty());
    EXPECT_TRUE(index.search(QStringLiteral("20260920")).isEmpty());
}

TEST(SearchIndexManager, UpdatesChangedNoteAndRemovesOldNgrams)
{
    VNoteItem note = makeNote(12, QStringLiteral("旧标题"), tiptapEnvelopeWithVoice());
    SearchIndexManager index;
    index.updateNote(&note);
    EXPECT_TRUE(index.search(QStringLiteral("语音转文字搜索")).isEmpty());

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

TEST(SearchIndexManager, RefreshFromNotesPicksUpSavedEditsWithoutExplicitUpdate)
{
    // 保存后不再单独调用 updateNote，搜索会话入口的 refreshFromNotes 必须能命中最新正文。
    VNoteItem note = makeNote(17, QStringLiteral("草稿标题"), QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[{"type":"text","text":"旧索引内容"}]}]}
    })json"));

    VNOTE_ITEMS_MAP folderNotes;
    folderNotes.folderNotes.insert(note.noteId, &note);
    VNOTE_ALL_NOTES_MAP allNotes;
    allNotes.notes.insert(note.folderId, &folderNotes);

    SearchIndexManager index;
    index.refreshFromNotes(&allNotes);
    ASSERT_EQ(index.search(QStringLiteral("旧索引内容")).size(), 1);

    // 模拟用户编辑并保存：只改数据库对象，不调用 SearchIndexManager::updateNote。
    note.modifyTime = note.modifyTime.addSecs(30);
    note.setMetadata(QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[{"type":"text","text":"全新保存内容"}]}]}
    })json"));

    index.refreshFromNotes(&allNotes);

    EXPECT_TRUE(index.search(QStringLiteral("旧索引内容")).isEmpty());
    const QList<SearchResult> results = index.search(QStringLiteral("全新保存内容"));
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().noteId, 17);
}

TEST(SearchIndexManager, RenameMakesNewTitleSearchableAndDropsOld)
{
    VNoteItem note = makeNote(18, QStringLiteral("旧标题关键词"), QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[{"type":"text","text":"正文"}]}]}
    })json"));

    SearchIndexManager index;
    index.updateNote(&note);
    ASSERT_EQ(index.search(QStringLiteral("旧标题关键词")).size(), 1);

    note.noteTitle = QStringLiteral("新标题关键词");
    note.modifyTime = note.modifyTime.addSecs(1);
    index.updateNote(&note);

    EXPECT_TRUE(index.search(QStringLiteral("旧标题关键词")).isEmpty());
    ASSERT_EQ(index.search(QStringLiteral("新标题关键词")).size(), 1);
    EXPECT_EQ(index.search(QStringLiteral("新标题关键词")).first().noteId, 18);
}

TEST(SearchIndexManager, RemovedNoteDisappearsFromIndex)
{
    VNoteItem note = makeNote(19, QStringLiteral("待删除笔记"), QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[{"type":"text","text":"删除前正文关键词"}]}]}
    })json"));

    SearchIndexManager index;
    index.updateNote(&note);
    ASSERT_EQ(index.search(QStringLiteral("删除前正文关键词")).size(), 1);

    index.removeNote(note.noteId);
    EXPECT_EQ(index.size(), 0);
    EXPECT_TRUE(index.search(QStringLiteral("删除前正文关键词")).isEmpty());
}

TEST(SearchIndexManager, ClearThenRebuildKeepsSearchWorking)
{
    VNoteItem note = makeNote(20, QStringLiteral("会话重建"), QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[{"type":"text","text":"会话重建正文关键词"}]}]}
    })json"));

    SearchIndexManager index;
    index.updateNote(&note);
    ASSERT_EQ(index.search(QStringLiteral("会话重建正文关键词")).size(), 1);

    // 退出搜索会释放索引，再次进入搜索应能重新建立。
    index.clear();
    EXPECT_EQ(index.size(), 0);
    EXPECT_TRUE(index.search(QStringLiteral("会话重建正文关键词")).isEmpty());

    index.refreshFromNotes(nullptr); // noteAll 为空时保持空的索引
    EXPECT_EQ(index.size(), 0);

    index.updateNote(&note);
    ASSERT_EQ(index.search(QStringLiteral("会话重建正文关键词")).size(), 1);
    EXPECT_EQ(index.search(QStringLiteral("会话重建正文关键词")).first().noteId, 20);
}

TEST(SearchIndexManager, SearchesOverlongNoteWithoutGramIndex)
{
    const QString body(1000005, QLatin1Char('a'));
    const QString envelope = QStringLiteral(R"json({
        "format":"tiptap",
        "schemaVersion":1,
        "content":{"type":"doc","content":[{"type":"paragraph","content":[{"type":"text","text":"%1"}]}]}
    })json").arg(body);
    VNoteItem note = makeNote(16, QStringLiteral("超长正文"), envelope);

    SearchIndexManager index;
    index.updateNote(&note);

    const QList<SearchResult> results = index.search(QStringLiteral("aaa"));
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().noteId, 16);
}

TEST(VNoteItemSearchCompatibility, DelegatesToUnifiedExtractor)
{
    VNoteItem note = makeNote(13, QStringLiteral("兼容入口"), tiptapEnvelopeWithVoice());
    EXPECT_FALSE(note.search(QStringLiteral("语音转文字搜索")));
    EXPECT_TRUE(note.search(QStringLiteral("tiptap 正文")));
    EXPECT_FALSE(note.search(QStringLiteral("不存在的关键字")));
}
