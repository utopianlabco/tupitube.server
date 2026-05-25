#ifndef SETTINGS_TEST_H
#define SETTINGS_TEST_H
#include <QtTest/QtTest>

class SettingsTest : public QObject
{
    Q_OBJECT
private slots:
    void test_repositoryPath();
    void test_backupPath();
    void test_emptyPath();
    void test_trailingSlash();
    void test_overwrite();
};
#endif // SETTINGS_TEST_H
