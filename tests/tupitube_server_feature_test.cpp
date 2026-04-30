#include "tupitube_server_feature_test.h"
#include "../src/server/modules/projects/databasehandler.h"
#include "../src/server/base/settings.h"
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDebug>
#include "../../tupitube.desk/src/framework/core/tapplicationproperties.h"
#include <QApplication>
#include <QTableWidget>
#include "../src/server/base/logger.h"

// Helper: initialize in-memory SQLite database for tests
static void initTestDatabase() {
    QSqlDatabase::addDatabase("QSQLITE");
    QSqlDatabase db = QSqlDatabase::database();
    db.setDatabaseName(":memory:");
    if (!db.open())
        qFatal("Failed to open test database: %s", qPrintable(db.lastError().text()));

    QString schemaPath = QCoreApplication::applicationDirPath() + "/../sql/schema_sqlite.sql";
    QFile schemaFile(schemaPath);
    if (schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList statements = QString(schemaFile.readAll()).split(';', Qt::SkipEmptyParts);
        for (const QString &stmt : statements) {
            if (!stmt.trimmed().isEmpty()) {
                QSqlQuery query(db);
                if (!query.exec(stmt))
                    qWarning() << "Schema statement failed:" << stmt << query.lastError();
            }
        }
    } else {
        qFatal("Failed to open schema file: %s", qPrintable(schemaPath));
    }
}

struct AppPropertiesInitializer {
    AppPropertiesInitializer() {
        TApplicationProperties::instance();
#ifdef TEST_PLUGINS_PATH
        kAppProp->setPluginDir(QString(TEST_PLUGINS_PATH));
#endif
    }
} appPropertiesInitializer;

struct AppNameSetter {
    AppNameSetter() { QCoreApplication::setApplicationName("tupitube_server"); }
} appNameSetter;

TupitubeServerFeatureTest::TupitubeServerFeatureTest() { initTestDatabase(); }

void TupitubeServerFeatureTest::cleanup() {
    // Wipe all data between tests in dependency-safe order (children first)
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    const QStringList tables = {
        "tupitube_chat",
        "tupitube_grade",
        "tupitube_collaboration",
        "tupitube_work",
        "tupitube_collection",
        "project_student",
        "tupitube_project",
        "tupitube_student",
        "student",
        "period",
        "class"
    };
    for (const QString &table : tables) {
        q.exec("DELETE FROM " + table);
        q.exec("DELETE FROM sqlite_sequence WHERE name='" + table + "'");
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Existence / smoke tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_DatabaseHandler_methods_exist() {
    DatabaseHandler db;
    QVERIFY(&DatabaseHandler::getAllStudents != nullptr);
    QVERIFY(&DatabaseHandler::addStudent != nullptr);
    QVERIFY(&DatabaseHandler::removeStudent != nullptr);
    QVERIFY(&DatabaseHandler::getAllClasses != nullptr);
    QVERIFY(&DatabaseHandler::addClass != nullptr);
    QVERIFY(&DatabaseHandler::removeClass != nullptr);
    QVERIFY(&DatabaseHandler::getAllPeriods != nullptr);
    QVERIFY(&DatabaseHandler::addPeriod != nullptr);
    QVERIFY(&DatabaseHandler::updatePeriod != nullptr);
    QVERIFY(&DatabaseHandler::removePeriod != nullptr);
    QVERIFY(&DatabaseHandler::getAllProjects != nullptr);
    QVERIFY(&DatabaseHandler::createEmptyProject != nullptr);
    QVERIFY(&DatabaseHandler::addCollaborator != nullptr);
    QVERIFY(&DatabaseHandler::removeCollaborator != nullptr);
}

// ──────────────────────────────────────────────────────────────────────────────
// CRUD tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_class_crud() {
    DatabaseHandler db;
    QVERIFY(db.addClass("TestClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto it = std::find_if(classes.begin(), classes.end(), [](const DatabaseHandler::ClassInfo& c){ return c.name == "TestClass"; });
    QVERIFY(it != classes.end());
    int classId = it->classId;
    QVERIFY(db.updateClass(classId, "TestClass2", 2027, "desc2"));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(), [classId](const DatabaseHandler::ClassInfo& c){ return c.classId == classId && c.name == "TestClass2"; });
    QVERIFY(it != classes.end());
    QVERIFY(db.removeClass(classId));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(), [classId](const DatabaseHandler::ClassInfo& c){ return c.classId == classId; });
    QVERIFY(it == classes.end());
}

void TupitubeServerFeatureTest::test_period_crud() {
    DatabaseHandler db;
    QVERIFY(db.addPeriod("TestPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "TestPeriod"; });
    QVERIFY(it != periods.end());
    int periodId = it->periodId;
    QVERIFY(db.updatePeriod(periodId, "TestPeriod2", 2027, "2027-01-01", "2027-12-31"));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(), [periodId](const DatabaseHandler::PeriodInfo& p){ return p.periodId == periodId && p.name == "TestPeriod2"; });
    QVERIFY(it != periods.end());
    QVERIFY(db.removePeriod(periodId));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(), [periodId](const DatabaseHandler::PeriodInfo& p){ return p.periodId == periodId; });
    QVERIFY(it == periods.end());
}

void TupitubeServerFeatureTest::test_student_crud() {
    DatabaseHandler db;
    QVERIFY(db.addStudent("teststudent", "Test Name", "pass", true, false, "TestClass"));
    auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "teststudent"; });
    QVERIFY(it != students.end());
    int studentId = it->studentId;
    QVERIFY(db.updateStudent(studentId, "teststudent2", "Test Name2", "pass2", false, true, "TestClass"));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(), [studentId](const DatabaseHandler::StudentInfo& s){ return s.studentId == studentId && s.studentname == "teststudent2"; });
    QVERIFY(it != students.end());
    QVERIFY(db.removeStudent(studentId));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(), [studentId](const DatabaseHandler::StudentInfo& s){ return s.studentId == studentId; });
    QVERIFY(it == students.end());
}

void TupitubeServerFeatureTest::test_project_crud() {
    DatabaseHandler db;
    QVERIFY(db.addClass("ProjClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto cit = std::find_if(classes.begin(), classes.end(), [](const DatabaseHandler::ClassInfo& c){ return c.name == "ProjClass"; });
    QVERIFY(cit != classes.end());
    QVERIFY(db.addStudent("projstudent", "Proj Name", "pass", true, false, "ProjClass"));
    auto students = db.getAllStudents();
    auto sit = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "projstudent"; });
    QVERIFY(sit != students.end());
    int ownerId = sit->studentId;
    QVERIFY(db.addPeriod("ProjPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto pit = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "ProjPeriod"; });
    QVERIFY(pit != periods.end());
    int periodId = pit->periodId;
    QList<int> collaborators;
    QVERIFY(db.createEmptyProject("TestProject", "desc", ownerId, "file.tup", collaborators, periodId));
    auto projects = db.getAllProjects();
    auto it = std::find_if(projects.begin(), projects.end(), [](const DatabaseHandler::ProjectRecord& p){ return p.title == "TestProject"; });
    QVERIFY(it != projects.end());
    int projectId = it->projectId;
    QVERIFY(db.deleteProject(projectId));
    projects = db.getAllProjects();
    it = std::find_if(projects.begin(), projects.end(), [projectId](const DatabaseHandler::ProjectRecord& p){ return p.projectId == projectId; });
    QVERIFY(it == projects.end());
    QVERIFY(db.removeStudent(ownerId));
    QVERIFY(db.removePeriod(periodId));
}

// ──────────────────────────────────────────────────────────────────────────────
// Class behavioral tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_addClass_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("BehaviorTestClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto it = std::find_if(classes.begin(), classes.end(), [](const DatabaseHandler::ClassInfo& c){ return c.name == "BehaviorTestClass"; });
    QVERIFY(it != classes.end());
}

void TupitubeServerFeatureTest::test_updateClass_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("UpdateClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto it = std::find_if(classes.begin(), classes.end(), [](const DatabaseHandler::ClassInfo& c){ return c.name == "UpdateClass"; });
    QVERIFY(it != classes.end());
    int classId = it->classId;
    QVERIFY(db.updateClass(classId, "UpdatedClass", 2027, "desc2"));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(), [classId](const DatabaseHandler::ClassInfo& c){ return c.classId == classId && c.name == "UpdatedClass"; });
    QVERIFY(it != classes.end());
}

void TupitubeServerFeatureTest::test_removeClass_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("RemoveClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto it = std::find_if(classes.begin(), classes.end(), [](const DatabaseHandler::ClassInfo& c){ return c.name == "RemoveClass"; });
    QVERIFY(it != classes.end());
    int classId = it->classId;
    QVERIFY(db.removeClass(classId));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(), [classId](const DatabaseHandler::ClassInfo& c){ return c.classId == classId; });
    QVERIFY(it == classes.end());
}

// ──────────────────────────────────────────────────────────────────────────────
// Period behavioral tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_addPeriod_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addPeriod("BehaviorPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "BehaviorPeriod"; });
    QVERIFY(it != periods.end());
}

void TupitubeServerFeatureTest::test_updatePeriod_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addPeriod("UpdatePeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "UpdatePeriod"; });
    QVERIFY(it != periods.end());
    int periodId = it->periodId;
    QVERIFY(db.updatePeriod(periodId, "UpdatedPeriod", 2027, "2027-01-01", "2027-12-31"));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(), [periodId](const DatabaseHandler::PeriodInfo& p){ return p.periodId == periodId && p.name == "UpdatedPeriod"; });
    QVERIFY(it != periods.end());
}

void TupitubeServerFeatureTest::test_removePeriod_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addPeriod("RemovePeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "RemovePeriod"; });
    QVERIFY(it != periods.end());
    int periodId = it->periodId;
    QVERIFY(db.removePeriod(periodId));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(), [periodId](const DatabaseHandler::PeriodInfo& p){ return p.periodId == periodId; });
    QVERIFY(it == periods.end());
}

// ──────────────────────────────────────────────────────────────────────────────
// Student behavioral tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_addStudent_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("StudentClass", 2026, "desc"));
    QVERIFY(db.addStudent("behaviorstudent", "Behavior Name", "pass", true, false, "StudentClass"));
    auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "behaviorstudent"; });
    QVERIFY(it != students.end());
}

void TupitubeServerFeatureTest::test_updateStudent_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("UpdateStudentClass", 2026, "desc"));
    QVERIFY(db.addStudent("updatestudent", "Update Name", "pass", true, false, "UpdateStudentClass"));
    auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "updatestudent"; });
    QVERIFY(it != students.end());
    int studentId = it->studentId;
    QVERIFY(db.updateStudent(studentId, "updatedstudent", "Updated Name", "pass2", false, true, "UpdateStudentClass"));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(), [studentId](const DatabaseHandler::StudentInfo& s){ return s.studentId == studentId && s.studentname == "updatedstudent"; });
    QVERIFY(it != students.end());
}

void TupitubeServerFeatureTest::test_removeStudent_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("RemoveStudentClass", 2026, "desc"));
    QVERIFY(db.addStudent("removestudent", "Remove Name", "pass", true, false, "RemoveStudentClass"));
    auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "removestudent"; });
    QVERIFY(it != students.end());
    int studentId = it->studentId;
    QVERIFY(db.removeStudent(studentId));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(), [studentId](const DatabaseHandler::StudentInfo& s){ return s.studentId == studentId; });
    QVERIFY(it == students.end());
}

void TupitubeServerFeatureTest::test_studentnameExists_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("ExistsClass", 2026, "desc"));
    QVERIFY(!db.studentnameExists("uniquestudent999"));
    QVERIFY(db.addStudent("uniquestudent999", "Unique Name", "pass", true, false, "ExistsClass"));
    QVERIFY(db.studentnameExists("uniquestudent999"));
}

// ──────────────────────────────────────────────────────────────────────────────
// Collaborator behavioral tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_addCollaborator_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("CollabClass", 2026, "desc"));
    QVERIFY(db.addStudent("ownerstudent", "Owner Name", "pass", true, false, "CollabClass"));
    QVERIFY(db.addStudent("collabstudent", "Collab Name", "pass", true, false, "CollabClass"));
    auto students = db.getAllStudents();
    auto ownerIt = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "ownerstudent"; });
    auto collabIt = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "collabstudent"; });
    QVERIFY(ownerIt != students.end() && collabIt != students.end());
    int ownerId = ownerIt->studentId;
    int collabId = collabIt->studentId;
    QVERIFY(db.addPeriod("CollabPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto pit = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "CollabPeriod"; });
    QVERIFY(pit != periods.end());
    int periodId = pit->periodId;
    QList<int> collaborators; collaborators << collabId;
    QVERIFY(db.createEmptyProject("CollabProject", "desc", ownerId, "file.tup", collaborators, periodId));
    auto projects = db.getAllProjects();
    auto projIt = std::find_if(projects.begin(), projects.end(), [](const DatabaseHandler::ProjectRecord& p){ return p.title == "CollabProject"; });
    QVERIFY(projIt != projects.end());
    int projectId = projIt->projectId;
    auto collabs = db.getProjectCollaborators(projectId);
    auto found = std::find_if(collabs.begin(), collabs.end(), [collabId](const DatabaseHandler::CollaboratorInfo& c){ return c.studentId == collabId; });
    QVERIFY(found != collabs.end());
}

void TupitubeServerFeatureTest::test_removeCollaborator_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("RemoveCollabClass", 2026, "desc"));
    QVERIFY(db.addStudent("ownerstudent2", "Owner2 Name", "pass", true, false, "RemoveCollabClass"));
    QVERIFY(db.addStudent("collabstudent2", "Collab2 Name", "pass", true, false, "RemoveCollabClass"));
    auto students = db.getAllStudents();
    auto ownerIt = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "ownerstudent2"; });
    auto collabIt = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "collabstudent2"; });
    QVERIFY(ownerIt != students.end() && collabIt != students.end());
    int ownerId = ownerIt->studentId;
    int collabId = collabIt->studentId;
    QVERIFY(db.addPeriod("RemoveCollabPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto pit = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "RemoveCollabPeriod"; });
    QVERIFY(pit != periods.end());
    int periodId = pit->periodId;
    QList<int> collaborators; collaborators << collabId;
    QVERIFY(db.createEmptyProject("RemoveCollabProject", "desc", ownerId, "file.tup", collaborators, periodId));
    auto projects = db.getAllProjects();
    auto projIt = std::find_if(projects.begin(), projects.end(), [](const DatabaseHandler::ProjectRecord& p){ return p.title == "RemoveCollabProject"; });
    QVERIFY(projIt != projects.end());
    int projectId = projIt->projectId;
    QVERIFY(db.removeCollaborator(projectId, collabId));
    auto collabs = db.getProjectCollaborators(projectId);
    auto found = std::find_if(collabs.begin(), collabs.end(), [collabId](const DatabaseHandler::CollaboratorInfo& c){ return c.studentId == collabId; });
    QVERIFY(found == collabs.end());
}

// ──────────────────────────────────────────────────────────────────────────────
// Project behavioral tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_createEmptyProject_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("ProjectClass", 2026, "desc"));
    QVERIFY(db.addStudent("projectowner", "Project Owner", "pass", true, false, "ProjectClass"));
    auto students = db.getAllStudents();
    auto ownerIt = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "projectowner"; });
    QVERIFY(ownerIt != students.end());
    int ownerId = ownerIt->studentId;
    QVERIFY(db.addPeriod("ProjectPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto pit = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "ProjectPeriod"; });
    QVERIFY(pit != periods.end());
    int periodId = pit->periodId;
    QList<int> collaborators;
    QVERIFY(db.createEmptyProject("BehaviorProject", "desc", ownerId, "file.tup", collaborators, periodId));
    auto projects = db.getAllProjects();
    auto projIt = std::find_if(projects.begin(), projects.end(), [](const DatabaseHandler::ProjectRecord& p){ return p.title == "BehaviorProject"; });
    QVERIFY(projIt != projects.end());
}

void TupitubeServerFeatureTest::test_getProjectInfo_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addClass("InfoClass", 2026, "desc"));
    QVERIFY(db.addStudent("infoowner", "Info Owner", "pass", true, false, "InfoClass"));
    auto students = db.getAllStudents();
    auto ownerIt = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "infoowner"; });
    QVERIFY(ownerIt != students.end());
    int ownerId = ownerIt->studentId;
    QVERIFY(db.addPeriod("InfoPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto pit = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "InfoPeriod"; });
    QVERIFY(pit != periods.end());
    int periodId = pit->periodId;
    QList<int> collaborators;
    QVERIFY(db.createEmptyProject("InfoProject", "desc", ownerId, "infofile.tup", collaborators, periodId));
    auto projects = db.getAllProjects();
    auto projIt = std::find_if(projects.begin(), projects.end(), [](const DatabaseHandler::ProjectRecord& p){ return p.title == "InfoProject"; });
    QVERIFY(projIt != projects.end());
    int projectId = projIt->projectId;
    QCOMPARE(db.getProjectFilename(projectId), QString("infofile.tup"));
    QCOMPARE(db.getProjectOwnerId(projectId), ownerId);
    QCOMPARE(db.getOwnerStudentname(projectId), QString("infoowner"));
}

// ──────────────────────────────────────────────────────────────────────────────
// Chat behavioral tests
// ──────────────────────────────────────────────────────────────────────────────

// Helper: create a minimal project and return its id
static int createChatTestProject(DatabaseHandler &db, const QString &suffix) {
    db.addClass("ChatClass" + suffix, 2026, "desc");
    db.addStudent("chatowner" + suffix, "Chat Owner " + suffix, "pass", true, false, "ChatClass" + suffix);
    auto students = db.getAllStudents();
    auto own = std::find_if(students.begin(), students.end(), [&suffix](const DatabaseHandler::StudentInfo& s){ return s.studentname == "chatowner" + suffix; });
    if (own == students.end()) return -1;
    int ownerId = own->studentId;
    db.addPeriod("ChatPeriod" + suffix, 2026, "2026-01-01", "2026-12-31");
    auto periods = db.getAllPeriods();
    auto pit = std::find_if(periods.begin(), periods.end(), [&suffix](const DatabaseHandler::PeriodInfo& p){ return p.name == "ChatPeriod" + suffix; });
    if (pit == periods.end()) return -1;
    int periodId = pit->periodId;
    QList<int> colls;
    db.createEmptyProject("ChatProject" + suffix, "desc", ownerId, "chat.tup", colls, periodId);
    auto projects = db.getAllProjects();
    auto proj = std::find_if(projects.begin(), projects.end(), [&suffix](const DatabaseHandler::ProjectRecord& p){ return p.title == "ChatProject" + suffix; });
    if (proj == projects.end()) return -1;
    return proj->projectId;
}

void TupitubeServerFeatureTest::test_saveChatMessage_behavior() {
    DatabaseHandler db;
    int projectId = createChatTestProject(db, "Save");
    QVERIFY(projectId > 0);
    auto students = db.getAllStudents();
    auto sit = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "chatownerSave"; });
    QVERIFY(sit != students.end());
    QVERIFY(db.saveChatMessage(projectId, sit->studentId, "chatownerSave", "Hello world", "chat"));
    auto history = db.getChatHistory(projectId);
    QVERIFY(!history.isEmpty());
    QCOMPARE(history.first().message, QString("Hello world"));
}

void TupitubeServerFeatureTest::test_getChatHistory_behavior() {
    DatabaseHandler db;
    int projectId = createChatTestProject(db, "Hist");
    QVERIFY(projectId > 0);
    auto students = db.getAllStudents();
    auto sit = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "chatownerHist"; });
    QVERIFY(sit != students.end());
    int studentId = sit->studentId;
    QVERIFY(db.saveChatMessage(projectId, studentId, "chatownerHist", "Msg 1", "chat"));
    QVERIFY(db.saveChatMessage(projectId, studentId, "chatownerHist", "Msg 2", "chat"));
    QVERIFY(db.saveChatMessage(projectId, studentId, "chatownerHist", "Msg 3", "notice"));
    auto all = db.getChatHistory(projectId);
    QVERIFY(all.size() >= 3);
    // limit test
    auto limited = db.getChatHistory(projectId, 2);
    QVERIFY(limited.size() <= 2);
}

void TupitubeServerFeatureTest::test_getChatHistoryByDate_behavior() {
    DatabaseHandler db;
    int projectId = createChatTestProject(db, "Date");
    QVERIFY(projectId > 0);
    auto students = db.getAllStudents();
    auto sit = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "chatownerDate"; });
    QVERIFY(sit != students.end());
    QVERIFY(db.saveChatMessage(projectId, sit->studentId, "chatownerDate", "Dated message", "chat"));
    // Query with a wide date range that includes today
    QString from = "2020-01-01";
    QString to   = "2099-12-31";
    auto history = db.getChatHistoryByDate(from, to);
    bool found = std::any_of(history.begin(), history.end(), [](const DatabaseHandler::ChatMessage& m){ return m.message == "Dated message"; });
    QVERIFY(found);
}

void TupitubeServerFeatureTest::test_clearChatHistory_behavior() {
    DatabaseHandler db;
    int projectId = createChatTestProject(db, "Clear");
    QVERIFY(projectId > 0);
    auto students = db.getAllStudents();
    auto sit = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "chatownerClear"; });
    QVERIFY(sit != students.end());
    QVERIFY(db.saveChatMessage(projectId, sit->studentId, "chatownerClear", "Will be cleared", "chat"));
    QVERIFY(!db.getChatHistory(projectId).isEmpty());
    QVERIFY(db.clearChatHistory(projectId));
    QVERIFY(db.getChatHistory(projectId).isEmpty());
}

// ──────────────────────────────────────────────────────────────────────────────
// Settings tests
// ──────────────────────────────────────────────────────────────────────────────

void TupitubeServerFeatureTest::test_settings_repositoryPath() {
    Settings *s = Settings::self();
    QVERIFY(s != nullptr);
    s->setRepositoryPath("/tmp/test_repo");
    QCOMPARE(s->repositoryPath(), QString("/tmp/test_repo/"));
}

void TupitubeServerFeatureTest::test_settings_backupPath() {
    Settings *s = Settings::self();
    QVERIFY(s != nullptr);
    s->setBackupPath("/tmp/test_backup");
    QCOMPARE(s->backupPath(), QString("/tmp/test_backup/"));
}
