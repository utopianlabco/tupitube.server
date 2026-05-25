#ifndef CLASS_TEST_H
#define CLASS_TEST_H
#include <QtTest/QtTest>

class ClassTest : public QObject
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
#endif // CLASS_TEST_H
