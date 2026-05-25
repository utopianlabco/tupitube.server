#include "grade_test.h"
#include "testhelpers.h"
#include "../src/server/modules/projects/databasehandler.h"

void GradeTest::initTestCase() { initTestDatabase(); }
void GradeTest::cleanup()      { cleanupDatabase(); }

// Internal helper: creates a minimal setup, returns a struct with IDs
struct GradeSetup { int projectId, studentId, periodId, classId; };

static GradeSetup makeGradeSetup(DatabaseHandler &db, const QString &suffix)
{
    GradeSetup s = {-1, -1, -1, -1};
    s.classId  = makeClass(db, "GradeClass" + suffix);
    if (s.classId < 0) return s;
    s.studentId = makeStudent(db, "gradeowner" + suffix, "Grade Owner", s.classId);
    if (s.studentId < 0) return s;
    s.periodId = makePeriod(db, "GradePeriod" + suffix);
    if (s.periodId < 0) return s;
    s.projectId = makeProject(db, "GradeProject" + suffix, s.studentId, s.periodId);
    return s;
}

void GradeTest::test_save()
{
    DatabaseHandler db;
    GradeSetup s = makeGradeSetup(db, "Save");
    QVERIFY2(s.projectId > 0, "Setup failed");

    QVERIFY(db.saveGrade(s.projectId, s.studentId, 0,
                         s.periodId, s.classId, "A", "Excellent work"));

    DatabaseHandler::GradeInfo gi = db.getGrade(s.projectId, s.studentId);
    QVERIFY(gi.found);
    QCOMPARE(gi.grade,    QString("A"));
    QCOMPARE(gi.comments, QString("Excellent work"));
}

void GradeTest::test_getNotFound()
{
    DatabaseHandler db;
    DatabaseHandler::GradeInfo gi = db.getGrade(99999, 99999);
    QVERIFY(!gi.found);
}

void GradeTest::test_update()
{
    DatabaseHandler db;
    GradeSetup s = makeGradeSetup(db, "Upd");
    QVERIFY2(s.projectId > 0, "Setup failed");

    QVERIFY(db.saveGrade(s.projectId, s.studentId, 0,
                         s.periodId, s.classId, "B", "Good"));
    QVERIFY(db.saveGrade(s.projectId, s.studentId, 0,
                         s.periodId, s.classId, "A+", "Outstanding"));

    DatabaseHandler::GradeInfo gi = db.getGrade(s.projectId, s.studentId);
    QVERIFY(gi.found);
    QCOMPARE(gi.grade,    QString("A+"));
    QCOMPARE(gi.comments, QString("Outstanding"));
}
