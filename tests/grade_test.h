#ifndef GRADE_TEST_H
#define GRADE_TEST_H
#include <QtTest/QtTest>

class GradeTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void test_save();
    void test_getNotFound();
    void test_update();
};
#endif // GRADE_TEST_H
