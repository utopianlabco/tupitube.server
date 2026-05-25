#ifndef PERIOD_TEST_H
#define PERIOD_TEST_H
#include <QtTest/QtTest>

class PeriodTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void test_crud();
    void test_add();
    void test_update();
    void test_remove();
};
#endif // PERIOD_TEST_H
