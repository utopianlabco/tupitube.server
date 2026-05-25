#ifndef PROJECT_TEST_H
#define PROJECT_TEST_H
#include <QtTest/QtTest>

class ProjectTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void test_crud();
    void test_create();
    void test_getInfo();
    void test_addCollaboratorAtCreation();
    void test_addCollaboratorStandalone();
    void test_removeCollaborator();
};
#endif // PROJECT_TEST_H
