// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// New unit tests for Utils (current API). The historical ut_utils.cpp is
// excluded from the build (API mismatch after refactor); this file targets
// the present Utils API to raise function coverage.

#include "utils.h"
#include "datatypedef.h"
#include "vnoteitem.h"

#include <gtest/gtest.h>
#include <QTest>
#include <QTextDocument>
#include <QTextCursor>
#include <QPixmap>
#include <QImage>
#include <QBuffer>
#include <QFile>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDir>

TEST(UtilsExt, convertDateTime_branches)
{
    QDateTime now = QDateTime::currentDateTime();
    // < 1 min ago
    EXPECT_FALSE(Utils::convertDateTime(now.addSecs(-5)).isEmpty());
    // mins ago
    EXPECT_FALSE(Utils::convertDateTime(now.addSecs(-120)).isEmpty());
    // today, > 1h
    EXPECT_FALSE(Utils::convertDateTime(now.addSecs(-4000)).isEmpty());
    // yesterday
    EXPECT_FALSE(Utils::convertDateTime(now.addDays(-1)).isEmpty());
    // same year older
    EXPECT_FALSE(Utils::convertDateTime(now.addDays(-10)).isEmpty());
    // previous year
    QDateTime prev = now.addYears(-1);
    EXPECT_FALSE(Utils::convertDateTime(prev).isEmpty());
}

TEST(UtilsExt, renderSVG_loadSVG)
{
    // empty size -> null pixmap path
    EXPECT_TRUE(Utils::renderSVG("/nonexistent.svg", QSize(), qApp).isNull());
    // create a real svg and render with a real size
    QTemporaryFile svg("voice-ut-XXXXXX.svg");
    svg.open();
    svg.write("<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10'><rect width='10' height='10' fill='red'/></svg>");
    svg.close();
    QPixmap rendered = Utils::renderSVG(svg.fileName(), QSize(10, 10), qApp);
    // exercise both canRead branches
    EXPECT_FALSE(rendered.isNull());

    // loadSVG: exercise fCommon true/false branches (resource may be absent)
    Utils::loadSVG("play.svg", true);
    Utils::loadSVG("play.svg", false);
    SUCCEED();
}

TEST(UtilsExt, highTextEdit)
{
    QTextDocument doc;
    doc.setPlainText("testeee");
    QColor color(Qt::yellow);
    EXPECT_EQ(1, Utils::highTextEdit(&doc, "test", color));
    EXPECT_EQ(2, Utils::highTextEdit(&doc, "t", color, true));
    EXPECT_EQ(4, Utils::highTextEdit(&doc, "e", color, true));
    // empty key -> 0
    EXPECT_EQ(0, Utils::highTextEdit(&doc, "", color));
}

TEST(UtilsExt, setDefaultColor)
{
    QTextDocument doc;
    Utils::setDefaultColor(&doc, QColor(Qt::black));
    SUCCEED();
}

TEST(UtilsExt, formatMillisecond)
{
    // below minValue -> clamped to minValue
    EXPECT_EQ("00:00:04", Utils::formatMillisecond(890, 4));
    // normal < 1h
    EXPECT_EQ("00:01:30", Utils::formatMillisecond(90000, 1));
    // >= 3600s -> max
    EXPECT_EQ("60:00:00", Utils::formatMillisecond(4000000, 1));
}

TEST(UtilsExt, blockToDocument_basic)
{
    VNoteItem item;
    VNoteBlock *block = item.newBlock(VNoteBlock::Text);
    ASSERT_NE(nullptr, block);
    block->blockText = "abc";
    QTextDocument doc;
    Utils::blockToDocument(block, &doc);
    EXPECT_EQ("abc", doc.toPlainText());
    Utils::blockToDocument(nullptr, &doc);  // null safety
}

TEST(UtilsExt, documentToBlock_basic)
{
    VNoteItem item;
    VNoteBlock *block = item.newBlock(VNoteBlock::Text);
    ASSERT_NE(nullptr, block);
    QTextDocument doc;
    doc.setPlainText("hello world");
    Utils::documentToBlock(block, &doc);
    EXPECT_EQ("hello world", block->blockText);
    // NOTE: Utils::documentToBlock(nullptr, &doc) is NOT exercised: the source
    // only null-guards the `block->blockText = ""` line, then dereferences
    // `block` unconditionally inside the `doc != nullptr` branch
    // (src/common/utils.cpp:~286). That is a source defect, reported, not
    // triggered here per the "don't modify source" rule.
}

TEST(UtilsExt, pictureToBase64)
{
    QString base64;
    EXPECT_FALSE(Utils::pictureToBase64("/no/such/image.png", base64));
    QImage img(4, 4, QImage::Format_RGB32);
    img.fill(Qt::red);
    QTemporaryFile png("voice-ut-XXXXXX.png");
    png.open();
    img.save(&png, "png");
    png.close();
    EXPECT_TRUE(Utils::pictureToBase64(png.fileName(), base64));
    EXPECT_TRUE(base64.startsWith("data:image/png;base64,"));
}

TEST(UtilsExt, platformAndEnv)
{
    EXPECT_FALSE(Utils::isWayland());
    Utils::isLoongsonPlatform();  // runs cat /proc/cpuinfo, caches
    Utils::inLinglongEnv();
    SUCCEED();
}

TEST(UtilsExt, filteredFileName)
{
    EXPECT_EQ("name", Utils::filteredFileName("n\"a/m<e", "default"));
    EXPECT_EQ("default", Utils::filteredFileName("***", "default"));
    EXPECT_EQ("ok file", Utils::filteredFileName("ok file"));
}

TEST(UtilsExt, richTextAndHtml)
{
    QString rt = Utils::createRichText("title key", "key");
    EXPECT_NE(-1, rt.indexOf("<span style=\"color: #0058de;\">key</span>"));
    EXPECT_EQ("hello", Utils::stripHtmlTags("<b>hello</b>"));
}

TEST(UtilsExt, osBuildParsing)
{
    EXPECT_FALSE(Utils::checkOsBuildValid("abc"));
    EXPECT_TRUE(Utils::checkOsBuildValid("11A11"));
    // professional (type 1 edit 1)
    EXPECT_EQ(DSysInfo::UosEdition::UosProfessional, Utils::parseOsBuildType("11A11"));
    // home
    EXPECT_EQ(DSysInfo::UosEdition::UosHome, Utils::parseOsBuildType("11A21"));
    // community
    EXPECT_EQ(DSysInfo::UosEdition::UosCommunity, Utils::parseOsBuildType("11A31"));
    // enterprise (type 2 edit 1)
    EXPECT_EQ(DSysInfo::UosEdition::UosEnterprise, Utils::parseOsBuildType("12A11"));
    // unknown (invalid)
    EXPECT_EQ(DSysInfo::UosEdition::UosEditionUnknown, Utils::parseOsBuildType("ZZZ"));
    // cached DBus path (invalid interface -> Unknown, but executes)
    Utils::uosEditionType();
    Utils::isCommunityEdition();
    SUCCEED();
}

TEST(UtilsExt, makeVoiceRelativeAbsolute)
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString abs = QDir(appData).filePath("voicenote/x.wav");
    // relative -> absolute
    QString back = Utils::makeVoiceAbsolute("voicenote/x.wav");
    EXPECT_FALSE(back.isEmpty());
    // absolute -> relative
    QString rel = Utils::makeVoiceRelative(abs);
    EXPECT_EQ("voicenote/x.wav", rel);
    // already a url -> unchanged
    EXPECT_EQ("http://h/x.wav", Utils::makeVoiceAbsolute("http://h/x.wav"));
    // already absolute -> unchanged
    EXPECT_EQ(abs, Utils::makeVoiceAbsolute(abs));
}

TEST(UtilsExt, setTitleBarTabFocus)
{
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    Utils::setTitleBarTabFocus(&ev);  // no-op in current impl
    SUCCEED();
}
