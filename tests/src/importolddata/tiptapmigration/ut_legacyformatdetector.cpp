// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/tiptapmigration/legacyformatdetector.h"

#include "gtest/gtest.h"

#include <QString>

namespace {

void expectFormat(const QString &payload, LegacyFormat expected)
{
    const QString original = payload;
    EXPECT_EQ(expected, LegacyFormatDetector::detect(payload));
    EXPECT_EQ(original, payload);
}

} // namespace

TEST(UT_LegacyFormatDetector, DetectsTiptapEnvelope)
{
    expectFormat(QStringLiteral(R"({"format":"tiptap","schemaVersion":1,"content":{"type":"doc","content":[]}})"),
                 LegacyFormat::TiptapEnvelope);
    expectFormat(QStringLiteral(R"(  {"format":"tiptap","content":{"type":"doc"}}  )"),
                 LegacyFormat::TiptapEnvelope);
}

TEST(UT_LegacyFormatDetector, DetectsProseMirrorDoc)
{
    expectFormat(QStringLiteral(R"({"type":"doc","content":[{"type":"paragraph"}]})"),
                 LegacyFormat::ProseMirrorDoc);
    expectFormat(QStringLiteral(R"({"type":"doc"})"), LegacyFormat::ProseMirrorDoc);
}

TEST(UT_LegacyFormatDetector, DetectsLegacyHtmlCode)
{
    expectFormat(QStringLiteral(R"({"htmlCode":"<p>hello</p>","meta_data":"{}"})"),
                 LegacyFormat::LegacyHtmlCode);
    expectFormat(QStringLiteral(R"(<div class="voiceBox" jsonKey="{}">voice</div>)"),
                 LegacyFormat::LegacyHtmlCode);
}

TEST(UT_LegacyFormatDetector, DetectsLegacyNoteDatas)
{
    expectFormat(QStringLiteral(R"({"dataCount":1,"noteDatas":[{"text":"hello","type":1}],"voiceMaxId":0})"),
                 LegacyFormat::LegacyNoteDatas);
    expectFormat(QStringLiteral(R"({"noteDatas":[{"title":"voice","type":2,"voicePath":"voicenote/a.mp3"}]})"),
                 LegacyFormat::LegacyNoteDatas);
}

TEST(UT_LegacyFormatDetector, DetectsPlainText)
{
    expectFormat(QStringLiteral("plain note text"), LegacyFormat::PlainText);
    expectFormat(QStringLiteral("中文 text with emoji"), LegacyFormat::PlainText);
}

TEST(UT_LegacyFormatDetector, DetectsInvalidDataWithoutCrash)
{
    expectFormat(QStringLiteral(R"({"format":"tiptap")"), LegacyFormat::Invalid);
    expectFormat(QStringLiteral(R"([{"type":"doc"})"), LegacyFormat::Invalid);
}

TEST(UT_LegacyFormatDetector, ReturnsStableFormatNames)
{
    EXPECT_EQ(QStringLiteral("TiptapEnvelope"), LegacyFormatDetector::formatName(LegacyFormat::TiptapEnvelope));
    EXPECT_EQ(QStringLiteral("Invalid"), LegacyFormatDetector::formatName(LegacyFormat::Invalid));
}
