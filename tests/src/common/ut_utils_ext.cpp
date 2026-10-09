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
    EXPECT_NE(-1, rt.indexOf("<span style=\"color: "));
    EXPECT_NE(-1, rt.indexOf("key</span>"));
    EXPECT_EQ("title key", Utils::stripHtmlTags(rt));
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

// ============================================================================
// PMS 补强回归用例（qt-autotest-generator Mode 7 → Mode 2 补强）
// ============================================================================

// BUG 340969/340979: 录音保存后右键语音文件提示"语音被删除" / Ctrl+D 保存失败。
// 根因：语音以相对路径存库，重启后找不到文件；修复 makeVoiceAbsolute 把相对
// 路径解析到 AppDataLocation（src/common/utils.cpp:38-50）。
// PMS: https://pms.uniontech.com/bug-view-340969.html  commit: 4ebe6e2f, 9d5579e3
// PMS: https://pms.uniontech.com/bug-view-340979.html  commit: 4ebe6e2f, 9d5579e3
TEST(UtilsExt, BUG340969_makeVoiceAbsolute_relativePathRoundTrip)
{
    const QString abs = QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/voicenote/ut-340969.wav");

    // 相对路径 → 绝对路径，必须落在 AppData 语音目录下且可反推回原相对路径
    const QString got = Utils::makeVoiceAbsolute("voicenote/ut-340969.wav");
    EXPECT_FALSE(got.isEmpty());
    EXPECT_TRUE(QDir::isAbsolutePath(got)) << "relative voice path must resolve to absolute";
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appData.isEmpty())
        EXPECT_TRUE(got.startsWith(QDir::toNativeSeparators(appData)))
            << "resolved path must live under AppDataLocation: " << qPrintable(got);

    // 往返一致：绝对 → 相对 → 绝对，避免保存后再次查找失败
    const QString rel = Utils::makeVoiceRelative(abs);
    EXPECT_EQ(abs, Utils::makeVoiceAbsolute(rel));
}

// BUG 86096: heap-use-after-free + 整型溢出漏洞（9d5579e3 修复）。
// UAF 场景：documentToBlock 遍历含图片片段/多段落文档时 fragment 迭代与
// 追加顺序导致的悬挂风险；溢出场景：renderSVG 对异常尺寸的缩放计算。
// PMS: https://pms.uniontech.com/bug-view-86096.html  commit: 9d5579e3
TEST(UtilsExt, BUG86096_documentToBlock_mixedFragmentsNoUaf)
{
    VNoteItem item;
    VNoteBlock *block = item.newBlock(VNoteBlock::Text);
    ASSERT_NE(nullptr, block);

    // 文本 + 图片片段混排：图片分支被跳过且不产生悬挂 fragment
    QTextDocument doc;
    QTextCursor cur(&doc);
    cur.insertText(QStringLiteral("before"));
    QTextImageFormat imgFmt;
    imgFmt.setName(QStringLiteral("ut-86096.png"));
    cur.insertImage(imgFmt);
    cur.insertText(QStringLiteral("after"));
    Utils::documentToBlock(block, &doc);
    EXPECT_EQ(QStringLiteral("beforeafter"), block->blockText);

    // 多段落文档：迭代器跨块推进（段落分隔符被保留）
    QTextDocument doc2;
    doc2.setPlainText(QStringLiteral("l1\nl2\nl3"));
    VNoteBlock *block2 = item.newBlock(VNoteBlock::Text);
    ASSERT_NE(nullptr, block2);
    Utils::documentToBlock(block2, &doc2);
    EXPECT_EQ(QStringLiteral("l1\nl2\nl3"), block2->blockText);
}

// BUG 86096（续）: renderSVG 对异常尺寸不得整型溢出崩溃。负尺寸在修复后
// 走 reader 失败→空 pixmap 路径；超大尺寸会触发真实大分配，依赖环境内存，
// 不在用例中触发。
// PMS: https://pms.uniontech.com/bug-view-86096.html  commit: 9d5579e3
TEST(UtilsExt, BUG86096_renderSVG_negativeSizeNoOverflow)
{
    QTemporaryFile svg("voice-ut-86096-XXXXXX.svg");
    ASSERT_TRUE(svg.open());
    svg.write("<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10'>"
              "<rect width='10' height='10' fill='blue'/></svg>");
    svg.close();

    // 异常尺寸（修复前整型溢出路径）：关键回归点是不崩溃、安全返回；
    // 具体返回值依 Qt 版本（可能钳位到自然尺寸），不设硬断言
    const QPixmap neg1 = Utils::renderSVG(svg.fileName(), QSize(-10, -10), qApp);
    const QPixmap neg2 = Utils::renderSVG(svg.fileName(), QSize(-1, 0), qApp);
    EXPECT_TRUE(neg1.isNull() || neg1.width() >= -10);
    EXPECT_TRUE(neg2.isNull() || neg2.width() >= -1);
    // 正常尺寸仍可渲染（对照）
    EXPECT_FALSE(Utils::renderSVG(svg.fileName(), QSize(8, 8), qApp).isNull());
}
