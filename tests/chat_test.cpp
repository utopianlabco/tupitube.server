#include "chat_test.h"
#include "testhelpers.h"
#include "../src/server/modules/projects/databasehandler.h"
#include <algorithm>

void ChatTest::initTestCase() { initTestDatabase(); }
void ChatTest::cleanup()      { cleanupDatabase(); }

// Internal helper: creates a minimal class/student/period/project, returns projectId
static int makeChatProject(DatabaseHandler &db, const QString &suffix)
{
    int classId  = makeClass(db, "ChatClass" + suffix);   if (classId  < 0) return -1;
    int ownerId  = makeStudent(db, "chatowner" + suffix, "Chat Owner", classId); if (ownerId < 0) return -1;
    int periodId = makePeriod(db, "ChatPeriod" + suffix); if (periodId < 0) return -1;
    return makeProject(db, "ChatProject" + suffix, ownerId, periodId);
}

void ChatTest::test_saveMessage()
{
    DatabaseHandler db;
    int projectId = makeChatProject(db, "Save");
    QVERIFY(projectId > 0);

    const auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(),
        [](const DatabaseHandler::StudentInfo &s){ return s.studentname == "chatownerSave"; });
    QVERIFY(it != students.end());

    QVERIFY(db.saveChatMessage(projectId, it->studentId, "chatownerSave", "Hello world", "chat"));
    const auto history = db.getChatHistory(projectId);
    QVERIFY(!history.isEmpty());
    QCOMPARE(history.first().message, QString("Hello world"));
}

void ChatTest::test_getHistory()
{
    DatabaseHandler db;
    int projectId = makeChatProject(db, "Hist");
    QVERIFY(projectId > 0);

    const auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(),
        [](const DatabaseHandler::StudentInfo &s){ return s.studentname == "chatownerHist"; });
    QVERIFY(it != students.end());
    int studentId = it->studentId;

    QVERIFY(db.saveChatMessage(projectId, studentId, "chatownerHist", "Msg 1", "chat"));
    QVERIFY(db.saveChatMessage(projectId, studentId, "chatownerHist", "Msg 2", "chat"));
    QVERIFY(db.saveChatMessage(projectId, studentId, "chatownerHist", "Msg 3", "notice"));

    QVERIFY(db.getChatHistory(projectId).size() >= 3);
    QVERIFY(db.getChatHistory(projectId, 2).size() <= 2);
}

void ChatTest::test_getHistoryByDate()
{
    DatabaseHandler db;
    int projectId = makeChatProject(db, "Date");
    QVERIFY(projectId > 0);

    const auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(),
        [](const DatabaseHandler::StudentInfo &s){ return s.studentname == "chatownerDate"; });
    QVERIFY(it != students.end());

    QVERIFY(db.saveChatMessage(projectId, it->studentId, "chatownerDate", "Dated message", "chat"));

    const auto history = db.getChatHistoryByDate("2020-01-01", "2099-12-31");
    QVERIFY(std::any_of(history.begin(), history.end(),
        [](const DatabaseHandler::ChatMessage &m){ return m.message == "Dated message"; }));
}

void ChatTest::test_clearHistory()
{
    DatabaseHandler db;
    int projectId = makeChatProject(db, "Clear");
    QVERIFY(projectId > 0);

    const auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(),
        [](const DatabaseHandler::StudentInfo &s){ return s.studentname == "chatownerClear"; });
    QVERIFY(it != students.end());

    QVERIFY(db.saveChatMessage(projectId, it->studentId, "chatownerClear", "Will be cleared", "chat"));
    QVERIFY(!db.getChatHistory(projectId).isEmpty());
    QVERIFY(db.clearChatHistory(projectId));
    QVERIFY(db.getChatHistory(projectId).isEmpty());
}
