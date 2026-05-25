#include "settings_test.h"
#include "../src/server/base/settings.h"

void SettingsTest::test_repositoryPath()
{
    Settings *s = Settings::self();
    QVERIFY(s != nullptr);
    s->setRepositoryPath("/tmp/test_repo");
    QCOMPARE(s->repositoryPath(), QString("/tmp/test_repo/"));
}

void SettingsTest::test_backupPath()
{
    Settings *s = Settings::self();
    QVERIFY(s != nullptr);
    s->setBackupPath("/tmp/test_backup");
    QCOMPARE(s->backupPath(), QString("/tmp/test_backup/"));
}

void SettingsTest::test_emptyPath()
{
    Settings *s = Settings::self();
    QVERIFY(s != nullptr);
    s->setRepositoryPath("");
    QCOMPARE(s->repositoryPath(), QString(""));
    s->setBackupPath("");
    QCOMPARE(s->backupPath(), QString(""));
}

void SettingsTest::test_trailingSlash()
{
    Settings *s = Settings::self();
    QVERIFY(s != nullptr);
    s->setRepositoryPath("/tmp/already_slashed/");
    QVERIFY2(!s->repositoryPath().endsWith("//"),
             "Repository path must not have a double trailing slash");
    s->setBackupPath("/tmp/backup_slashed/");
    QVERIFY2(!s->backupPath().endsWith("//"),
             "Backup path must not have a double trailing slash");
}

void SettingsTest::test_overwrite()
{
    Settings *s = Settings::self();
    QVERIFY(s != nullptr);
    s->setRepositoryPath("/tmp/first");
    s->setRepositoryPath("/tmp/second");
    QCOMPARE(s->repositoryPath(), QString("/tmp/second/"));
}
