// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Covers default constructors and trivial path getters of the dbmigration
// helper classes that the existing per-class suites reach via their
// parameterized constructors only.

#include "importolddata/dbmigration/migrationbackup.h"
#include "importolddata/dbmigration/migrationreport.h"
#include "importolddata/dbmigration/migrationscanner.h"
#include "importolddata/dbmigration/migrationwriter.h"
#include "importolddata/dbmigration/migrationstatepersistent.h"
#include "importolddata/dbmigration/migrationstate.h"

#include <gtest/gtest.h>

TEST(DbMigrationMiscUT, defaultCtorsAndGetters)
{
    MigrationBackup backup;
    EXPECT_TRUE(backup.dbPath().isEmpty());
    EXPECT_TRUE(backup.backupDir().isEmpty());

    MigrationReport report;
    EXPECT_TRUE(report.reportDir().isEmpty());

    MigrationScanner scanner;
    EXPECT_TRUE(scanner.dbPath().isEmpty());

    MigrationWriter writer;
    EXPECT_TRUE(writer.dbPath().isEmpty());

    MigrationStatePersistent persistent("/tmp/voice-ut-state.json");
    EXPECT_EQ("/tmp/voice-ut-state.json", persistent.filePath());

    MigrationStateMachine machine;
    (void)machine;
    SUCCEED();
}
