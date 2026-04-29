#include "tupitube_server_feature_test.h"
#include "../src/server/modules/projects/databasehandler.h"
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

void TupitubeServerFeatureTest::test_addPeriod_behavior() {
    DatabaseHandler db;
    QVERIFY(db.addPeriod("BehaviorPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "BehaviorPeriod"; });
    QVERIFY(it != periods.end());
}

#include "tupitube_server_feature_test.h"
#include "../src/server/modules/projects/databasehandler.h"
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

// ...existing code continues (implementations start here)...

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
    QList<int> collaborators; // none
    QVERIFY(db.createEmptyProject("BehaviorProject", "desc", ownerId, "file.tup", collaborators, periodId));
    auto projects = db.getAllProjects();
    auto projIt = std::find_if(projects.begin(), projects.end(), [](const DatabaseHandler::ProjectRecord& p){ return p.title == "BehaviorProject"; });
    QVERIFY(projIt != projects.end());
}
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDebug>

// Helper to initialize test database
void initTestDatabase() {
    QSqlDatabase::addDatabase("QSQLITE");
    QSqlDatabase db = QSqlDatabase::database();
    db.setDatabaseName(":memory:");
    if (!db.open()) {
        qFatal("Failed to open test database: %s", qPrintable(db.lastError().text()));
    }

        QString schemaPath = QCoreApplication::applicationDirPath() + "/../sql/schema_sqlite.sql";
        QFile schemaFile(schemaPath);
    if (schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList statements = QString(schemaFile.readAll()).split(';', Qt::SkipEmptyParts);
        for (const QString &stmt : statements) {
            if (!stmt.trimmed().isEmpty()) {
                QSqlQuery query(db);
                if (!query.exec(stmt)) {
                    qWarning() << "Failed to execute schema statement:" << stmt << query.lastError();
                }
            }
        }
    } else {
        qFatal("Failed to open schema file");
    }
}
#include "../../tupitube.desk/src/framework/core/tapplicationproperties.h"
// Ensure the TApplicationProperties singleton is initialized for PLUGINS_DIR macro
struct AppPropertiesInitializer {
    AppPropertiesInitializer() {
        TApplicationProperties::instance(); // Safely initializes the singleton
    #ifdef TEST_PLUGINS_PATH
        kAppProp->setPluginDir(QString(TEST_PLUGINS_PATH));
    #endif
    }
} appPropertiesInitializer;
#include "tupitube_server_feature_test.h"
#include "../src/server/modules/projects/databasehandler.h"
#include <QApplication>
#include <QTableWidget>

#include "../src/server/base/logger.h"


TupitubeServerFeatureTest::TupitubeServerFeatureTest() { initTestDatabase(); }


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

void TupitubeServerFeatureTest::test_class_crud() {

    DatabaseHandler db;
    // Create
    QVERIFY(db.addClass("TestClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto it = std::find_if(classes.begin(), classes.end(), [](const DatabaseHandler::ClassInfo& c){ return c.name == "TestClass"; });
    QVERIFY(it != classes.end());
    int classId = it->classId;
    // Edit
    QVERIFY(db.updateClass(classId, "TestClass2", 2027, "desc2"));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(), [classId](const DatabaseHandler::ClassInfo& c){ return c.classId == classId && c.name == "TestClass2"; });
    QVERIFY(it != classes.end());
    // Remove
    QVERIFY(db.removeClass(classId));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(), [classId](const DatabaseHandler::ClassInfo& c){ return c.classId == classId; });
    QVERIFY(it == classes.end());
}

void TupitubeServerFeatureTest::test_period_crud() {
    DatabaseHandler db;
    // Create
    QVERIFY(db.addPeriod("TestPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "TestPeriod"; });
    QVERIFY(it != periods.end());
    int periodId = it->periodId;
    // Edit
    QVERIFY(db.updatePeriod(periodId, "TestPeriod2", 2027, "2027-01-01", "2027-12-31"));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(), [periodId](const DatabaseHandler::PeriodInfo& p){ return p.periodId == periodId && p.name == "TestPeriod2"; });
    QVERIFY(it != periods.end());
    // Remove
    QVERIFY(db.removePeriod(periodId));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(), [periodId](const DatabaseHandler::PeriodInfo& p){ return p.periodId == periodId; });
    QVERIFY(it == periods.end());
}

void TupitubeServerFeatureTest::test_student_crud() {
    DatabaseHandler db;
    // Create
    QVERIFY(db.addStudent("teststudent", "Test Name", "pass", true, false, "TestClass"));
    auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "teststudent"; });
    QVERIFY(it != students.end());
    int studentId = it->studentId;
    // Edit
    QVERIFY(db.updateStudent(studentId, "teststudent2", "Test Name2", "pass2", false, true, "TestClass"));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(), [studentId](const DatabaseHandler::StudentInfo& s){ return s.studentId == studentId && s.studentname == "teststudent2"; });
    QVERIFY(it != students.end());
    // Remove
    QVERIFY(db.removeStudent(studentId));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(), [studentId](const DatabaseHandler::StudentInfo& s){ return s.studentId == studentId; });
    QVERIFY(it == students.end());
}

void TupitubeServerFeatureTest::test_project_crud() {
    DatabaseHandler db;
    // For project, we need at least one class, student, and period
    QVERIFY(db.addClass("ProjClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto cit = std::find_if(classes.begin(), classes.end(), [](const DatabaseHandler::ClassInfo& c){ return c.name == "ProjClass"; });
    QVERIFY(cit != classes.end());
    int classId = cit->classId;
    qDebug() << "[test_project_crud] classId:" << classId;
    QVERIFY(db.addStudent("projstudent", "Proj Name", "pass", true, false, "ProjClass"));
    auto students = db.getAllStudents();
    auto sit = std::find_if(students.begin(), students.end(), [](const DatabaseHandler::StudentInfo& s){ return s.studentname == "projstudent"; });
    QVERIFY(sit != students.end());
    int ownerId = sit->studentId;
    qDebug() << "[test_project_crud] ownerId:" << ownerId;
    QVERIFY(db.addPeriod("ProjPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto pit = std::find_if(periods.begin(), periods.end(), [](const DatabaseHandler::PeriodInfo& p){ return p.name == "ProjPeriod"; });
    QVERIFY(pit != periods.end());
    int periodId = pit->periodId;
    qDebug() << "[test_project_crud] periodId:" << periodId;
    // ...existing code...
    // Create
    QList<int> collaborators; // empty for now
    QVERIFY(db.createEmptyProject("TestProject", "desc", ownerId, "file.tup", collaborators, periodId));
    auto projects = db.getAllProjects();
    auto it = std::find_if(projects.begin(), projects.end(), [](const DatabaseHandler::ProjectRecord& p){ return p.title == "TestProject"; });
    QVERIFY(it != projects.end());
    int projectId = it->projectId;
    // Remove
    QVERIFY(db.deleteProject(projectId));
    projects = db.getAllProjects();
    it = std::find_if(projects.begin(), projects.end(), [projectId](const DatabaseHandler::ProjectRecord& p){ return p.projectId == projectId; });
    QVERIFY(it == projects.end());
    // Cleanup
    QVERIFY(db.removeStudent(ownerId));
    QVERIFY(db.removePeriod(periodId));
}

#include <QCoreApplication>
// Ensure the config path is ~/.tupitube_server/tupitube_server.cfg for tests
struct AppNameSetter {
    AppNameSetter() { QCoreApplication::setApplicationName("tupitube_server"); }
} appNameSetter;

// QTEST_MAIN removed: unified main is now in main.cpp
