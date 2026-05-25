#ifndef STUDENT_TEST_H
#define STUDENT_TEST_H
#include <QtTest/QtTest>

class StudentTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void test_crud();
    void test_add();
    void test_update();
    void test_remove();
    void test_studentnameExists();
};
#endif // STUDENT_TEST_H
