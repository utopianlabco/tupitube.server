#ifndef TUPITUBE_SERVER_FEATURE_TEST_H
#define TUPITUBE_SERVER_FEATURE_TEST_H
#include <QtTest/QtTest>
class TupitubeServerFeatureTest : public QObject {
    Q_OBJECT
public:
    TupitubeServerFeatureTest();
private Q_SLOTS:
    void cleanup();
    void test_DatabaseHandler_methods_exist();
    void test_class_crud();
    void test_period_crud();
    void test_student_crud();
    void test_project_crud();
    void test_addClass_behavior();
    void test_updateClass_behavior();
    void test_removeClass_behavior();
    void test_addPeriod_behavior();
    void test_updatePeriod_behavior();
    void test_removePeriod_behavior();
    void test_addStudent_behavior();
    void test_updateStudent_behavior();
    void test_removeStudent_behavior();
    void test_studentnameExists_behavior();
    void test_addCollaborator_behavior();
    void test_removeCollaborator_behavior();
    void test_createEmptyProject_behavior();
    void test_getProjectInfo_behavior();
    void test_saveChatMessage_behavior();
    void test_getChatHistory_behavior();
    void test_getChatHistoryByDate_behavior();
    void test_clearChatHistory_behavior();
    void test_settings_repositoryPath();
    void test_settings_backupPath();
};
#endif // TUPITUBE_SERVER_FEATURE_TEST_H