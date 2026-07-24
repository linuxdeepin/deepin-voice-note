// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importolddata/tiptapmigration/migrationhtmlconverter.h"
#include "importolddata/tiptapmigration/migrationjsonbuilder.h"
#include "importolddata/tiptapmigration/migrationjsonvalidator.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QVector>

#include <algorithm>

namespace {

// Absolute source-tree path injected by tests/CMakeLists.txt.
#ifndef MIGRATION_HTML_FIXTURES_DIR
#error "MIGRATION_HTML_FIXTURES_DIR must be defined by the build system"
#endif

QString fixturesDir()
{
    return QString::fromUtf8(MIGRATION_HTML_FIXTURES_DIR);
}

QString readFile(const QString &path, bool *ok)
{
    *ok = false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    const QByteArray bytes = file.readAll();
    file.close();
    *ok = true;
    return QString::fromUtf8(bytes);
}

QJsonDocument readJsonDocument(const QString &path, bool *ok)
{
    *ok = false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonDocument();
    }
    const QByteArray bytes = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return QJsonDocument();
    }

    *ok = true;
    return document;
}

bool writeJsonDocument(const QString &path, const QJsonDocument &document)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(document.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QJsonArray docContentOf(const QJsonObject &envelope)
{
    return envelope.value(QStringLiteral("content")).toObject().value(QStringLiteral("content")).toArray();
}

// A (code, path-fragment) expectation entry decoded from warnings.json/errors.json.
// `path` may be empty, in which case only the code multiset is asserted.
struct IssueExpectation {
    QString code;
    QString path;
};

QVector<IssueExpectation> decodeIssueExpectations(const QJsonArray &array)
{
    QVector<IssueExpectation> expectations;
    for (const QJsonValue &value : array) {
        const QJsonObject entry = value.toObject();
        IssueExpectation expectation;
        expectation.code = entry.value(QStringLiteral("code")).toString();
        expectation.path = entry.value(QStringLiteral("path")).toString();
        expectations.append(expectation);
    }
    return expectations;
}

// Returns the multiset of warning/error codes actually produced by convert().
QStringList actualCodeMultiset(const QVector<MigrationHtmlConversionIssue> &issues)
{
    QStringList codes;
    codes.reserve(issues.size());
    for (const MigrationHtmlConversionIssue &issue : issues) {
        codes.append(issue.code);
    }
    std::sort(codes.begin(), codes.end());
    return codes;
}

QStringList expectedCodeMultiset(const QVector<IssueExpectation> &expectations)
{
    QStringList codes;
    codes.reserve(expectations.size());
    for (const IssueExpectation &expectation : expectations) {
        codes.append(expectation.code);
    }
    std::sort(codes.begin(), codes.end());
    return codes;
}

// For each expected entry, ensure some actual issue shares its code and (when a
// path fragment is pinned) the actual path contains that fragment.
::testing::AssertionResult issuesMatch(const QVector<MigrationHtmlConversionIssue> &actual,
                                       const QVector<IssueExpectation> &expected,
                                       const QString &label)
{
    const QStringList actualCodes = actualCodeMultiset(actual);
    const QStringList expectedCodes = expectedCodeMultiset(expected);
    if (actualCodes != expectedCodes) {
        ::testing::AssertionResult result = ::testing::AssertionFailure()
            << label.toStdString() << " code multiset mismatch.\n"
            << "  expected codes: " << expectedCodes.join(QStringLiteral(", ")).toStdString() << "\n"
            << "  actual codes:   " << actualCodes.join(QStringLiteral(", ")).toStdString();
        return result;
    }

    for (const IssueExpectation &expectation : expected) {
        if (expectation.path.isEmpty()) {
            continue;
        }

        bool found = false;
        for (const MigrationHtmlConversionIssue &issue : actual) {
            if (issue.code == expectation.code && issue.path.contains(expectation.path)) {
                found = true;
                break;
            }
        }

        if (!found) {
            std::string actualPaths;
            for (const MigrationHtmlConversionIssue &issue : actual) {
                if (issue.code == expectation.code) {
                    actualPaths += "\n    " + issue.path.toStdString();
                }
            }
            return ::testing::AssertionFailure()
                << label.toStdString() << " code '" << expectation.code.toStdString()
                << "' expected a path containing '" << expectation.path.toStdString()
                << "' but no matching issue was produced."
                << "\n  actual paths for this code:" << actualPaths;
        }
    }

    return ::testing::AssertionSuccess();
}

QJsonObject issueToObject(const MigrationHtmlConversionIssue &issue)
{
    QJsonObject object;
    object.insert(QStringLiteral("code"), issue.code);
    object.insert(QStringLiteral("path"), issue.path);
    return object;
}

QJsonArray issuesToArray(const QVector<MigrationHtmlConversionIssue> &issues)
{
    QJsonArray array;
    for (const MigrationHtmlConversionIssue &issue : issues) {
        array.append(issueToObject(issue));
    }
    return array;
}

// Regenerate mode: write actual convert() output back to the fixture files so
// the golden snapshots can be (re)captured from the real converter. This is the
// documented workflow ("run convert(), capture output, review, persist").
void regenerateFixture(const QString &dir, const MigrationHtmlConversionResult &result)
{
    writeJsonDocument(dir + QStringLiteral("/golden.json"),
                      QJsonDocument(result.envelope));
    writeJsonDocument(dir + QStringLiteral("/warnings.json"),
                      QJsonDocument(issuesToArray(result.warnings)));
    writeJsonDocument(dir + QStringLiteral("/errors.json"),
                      QJsonDocument(issuesToArray(result.errors)));
}

} // namespace
// Per-fixture processing in a helper so an ASSERT failure only abandons the
// current fixture and the loop continues to the next one.
void processFixture(const QString &name, bool regenMode)
{
    SCOPED_TRACE(name.toStdString());

    const QDir root(fixturesDir());
    const QString dir = root.absoluteFilePath(name);

    bool inputOk = false;
    const QString inputHtml = readFile(dir + QStringLiteral("/input.html"), &inputOk);
    ASSERT_TRUE(inputOk) << "Cannot read input.html for fixture " << name.toStdString();

    const MigrationHtmlConversionResult result = MigrationHtmlConverter::convert(inputHtml);

    if (regenMode) {
        regenerateFixture(dir, result);
        return;
    }

    bool goldenOk = false;
    const QJsonDocument goldenDoc = readJsonDocument(dir + QStringLiteral("/golden.json"), &goldenOk);
    ASSERT_TRUE(goldenOk && goldenDoc.isObject())
        << "Cannot parse golden.json for fixture " << name.toStdString();

    const QJsonObject expectedEnvelope = goldenDoc.object();

    // (1) Envelope deep-equal.
    EXPECT_EQ(result.envelope, expectedEnvelope)
        << "Envelope mismatch for fixture " << name.toStdString()
        << "\n  expected: " << MigrationJsonBuilder::toCompactJson(expectedEnvelope).toStdString()
        << "\n  actual:   " << MigrationJsonBuilder::toCompactJson(result.envelope).toStdString();

    // (4) Envelope validity for non-error fixtures.
    if (result.errors.isEmpty()) {
        const MigrationJsonValidationResult validation =
            MigrationJsonValidator::validateEnvelope(result.envelope);
        EXPECT_TRUE(validation.ok())
            << "Envelope failed validation for fixture " << name.toStdString()
            << (validation.errors.isEmpty()
                    ? std::string()
                    : (QStringLiteral(": ") + validation.errors.constFirst().code
                       + QStringLiteral(" @ ") + validation.errors.constFirst().path).toStdString());
    }

    // (2) Warnings: code multiset + path-fragment containment.
    bool warningsOk = false;
    QJsonArray warningsArray = readJsonDocument(dir + QStringLiteral("/warnings.json"), &warningsOk)
        .array();
    if (!warningsOk) {
        warningsArray = QJsonArray();
    }
    const QVector<IssueExpectation> expectedWarnings = decodeIssueExpectations(warningsArray);
    EXPECT_TRUE(issuesMatch(result.warnings, expectedWarnings, QStringLiteral("warnings")))
        << "Warnings mismatch for fixture " << name.toStdString();

    // (3) Errors: code multiset + path-fragment containment.
    bool errorsOk = false;
    QJsonArray errorsArray = readJsonDocument(dir + QStringLiteral("/errors.json"), &errorsOk)
        .array();
    if (!errorsOk) {
        errorsArray = QJsonArray();
    }
    const QVector<IssueExpectation> expectedErrors = decodeIssueExpectations(errorsArray);
    EXPECT_TRUE(issuesMatch(result.errors, expectedErrors, QStringLiteral("errors")))
        << "Errors mismatch for fixture " << name.toStdString();
}

// File-driven golden regression harness for MigrationHtmlConverter::convert().
//
// Each fixture is a subdirectory of MIGRATION_HTML_FIXTURES_DIR containing:
//   input.html     - sanitized legacy HTML to convert
//   golden.json    - expected envelope (deep-equal)
//   warnings.json  - expected warnings [{code, path?}]
//   errors.json    - expected errors   [{code, path?}] (optional, defaults to [])
//
// New fixtures are picked up automatically; no test code or CMake source list
// changes are required.
//
// Golden capture mode: set the environment variable
//   MIGRATION_HTML_FIXTURES_REGEN=1
// to overwrite golden.json/warnings.json/errors.json with the actual convert()
// output for every fixture (no comparison is performed). Use this to bootstrap
// or refresh snapshots, then review the diff before committing.
TEST(UT_MigrationHtmlFixtures, GoldenRegression)
{
    const bool regenMode = QProcessEnvironment::systemEnvironment()
        .value(QStringLiteral("MIGRATION_HTML_FIXTURES_REGEN")).compare(
            QStringLiteral("1"), Qt::CaseInsensitive) == 0;

    const QDir root(fixturesDir());
    ASSERT_TRUE(root.exists()) << "Fixture root not found: " << fixturesDir().toStdString();

    const QStringList entries = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    ASSERT_FALSE(entries.isEmpty()) << "No fixture subdirectories under " << fixturesDir().toStdString();

    for (const QString &name : entries) {
        processFixture(name, regenMode);
    }

    if (regenMode) {
        GTEST_SKIP() << "Golden snapshots regenerated in place; review the diff before committing.";
    }
}
