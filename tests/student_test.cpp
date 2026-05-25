#include "student_test.h"
#include "testhelpers.h"
#include "../src/server/modules/projects/databasehandler.h"
#include <algorithm>

void StudentTest::initTestCase() { initTestDatabase(); }
void StudentTest::cleanup()      { cleanupDatabase(); }

void StudentTest::test_crud()
{
    DatabaseHandler db;
    int classId = makeClass(db, "StudentCrudClass");
    QVERIFY(classId > 0);

    QVERIFY(db.addStudent("crudstudent", "Crud Name", "pass", true, false, classId));
    auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(),
        [](const DatabaseHandler::StudentInfo &s){ return s.studentname == "crudstudent"; });
    QVERIFY(it != students.end());
    int id = it->studentId;

    QVERIFY(db.updateStudent(id, "crudstudent2", "Crud Name2", "pass2", false, true, classId));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(),
        [id](const DatabaseHandler::StudentInfo &s){
            return s.studentId == id && s.studentname == "crudstudent2";
        });
    QVERIFY(it != students.end());

    QVERIFY(db.removeStudent(id));
    students = db.getAllStudents();
    it = std::find_if(students.begin(), students.end(),
        [id](const DatabaseHandler::StudentInfo &s){ return s.studentId == id; });
    QVERIFY(it == students.end());
}

void StudentTest::test_add()
{
    DatabaseHandler db;
    int classId = makeClass(db, "AddStudentClass");
    QVERIFY(classId > 0);
    QVERIFY(db.addStudent("addstudent", "Add Name", "pass", true, false, classId));
    const auto students = db.getAllStudents();
    QVERIFY(std::any_of(students.begin(), students.end(),
        [](const DatabaseHandler::StudentInfo &s){ return s.studentname == "addstudent"; }));
}

void StudentTest::test_update()
{
    DatabaseHandler db;
    int classId = makeClass(db, "UpdateStudentClass");
    QVERIFY(classId > 0);
    int id = makeStudent(db, "beforeupdate", "Before", classId);
    QVERIFY(id > 0);
    QVERIFY(db.updateStudent(id, "afterupdate", "After", "newpass", false, true, classId));
    const auto students = db.getAllStudents();
    QVERIFY(std::any_of(students.begin(), students.end(),
        [id](const DatabaseHandler::StudentInfo &s){
            return s.studentId == id && s.studentname == "afterupdate";
        }));
}

void StudentTest::test_remove()
{
    DatabaseHandler db;
    int classId = makeClass(db, "RemoveStudentClass");
    QVERIFY(classId > 0);
    int id = makeStudent(db, "removestudent", "Remove Name", classId);
    QVERIFY(id > 0);
    QVERIFY(db.removeStudent(id));
    const auto students = db.getAllStudents();
    QVERIFY(std::none_of(students.begin(), students.end(),
        [id](const DatabaseHandler::StudentInfo &s){ return s.studentId == id; }));
}

void StudentTest::test_studentnameExists()
{
    DatabaseHandler db;
    int classId = makeClass(db, "ExistsClass");
    QVERIFY(classId > 0);
    QVERIFY(!db.studentnameExists("uniquestudent999"));
    QVERIFY(db.addStudent("uniquestudent999", "Unique", "pass", true, false, classId));
    QVERIFY(db.studentnameExists("uniquestudent999"));
}
