#include "project_test.h"
#include "testhelpers.h"
#include "../src/server/modules/projects/databasehandler.h"
#include <algorithm>

void ProjectTest::initTestCase() { initTestDatabase(); }
void ProjectTest::cleanup()      { cleanupDatabase(); }

void ProjectTest::test_crud()
{
    DatabaseHandler db;
    int classId  = makeClass(db, "ProjCrudClass");   QVERIFY(classId  > 0);
    int ownerId  = makeStudent(db, "projcrudowner", "Crud Owner", classId); QVERIFY(ownerId > 0);
    int periodId = makePeriod(db, "ProjCrudPeriod"); QVERIFY(periodId > 0);

    int projectId = makeProject(db, "CrudProject", ownerId, periodId);
    QVERIFY(projectId > 0);

    QVERIFY(db.deleteProject(projectId));
    const auto projects = db.getAllProjects();
    QVERIFY(std::none_of(projects.begin(), projects.end(),
        [projectId](const DatabaseHandler::ProjectRecord &p){ return p.projectId == projectId; }));
}

void ProjectTest::test_create()
{
    DatabaseHandler db;
    int classId  = makeClass(db, "CreateProjClass");   QVERIFY(classId  > 0);
    int ownerId  = makeStudent(db, "createprojowner", "Create Owner", classId); QVERIFY(ownerId > 0);
    int periodId = makePeriod(db, "CreateProjPeriod"); QVERIFY(periodId > 0);

    int projectId = makeProject(db, "NewProject", ownerId, periodId);
    QVERIFY(projectId > 0);
    const auto projects = db.getAllProjects();
    QVERIFY(std::any_of(projects.begin(), projects.end(),
        [](const DatabaseHandler::ProjectRecord &p){ return p.title == "NewProject"; }));
}

void ProjectTest::test_getInfo()
{
    DatabaseHandler db;
    int classId  = makeClass(db, "InfoProjClass");   QVERIFY(classId  > 0);
    int ownerId  = makeStudent(db, "infoprojowner", "Info Owner", classId); QVERIFY(ownerId > 0);
    int periodId = makePeriod(db, "InfoProjPeriod"); QVERIFY(periodId > 0);

    QVERIFY(db.createEmptyProject("InfoProject", "desc", ownerId,
                                  "infofile.tup", {}, periodId));
    const auto projects = db.getAllProjects();
    auto it = std::find_if(projects.begin(), projects.end(),
        [](const DatabaseHandler::ProjectRecord &p){ return p.title == "InfoProject"; });
    QVERIFY(it != projects.end());
    int projectId = it->projectId;

    QCOMPARE(db.getProjectFilename(projectId), QString("infofile.tup"));
    QCOMPARE(db.getProjectOwnerId(projectId), ownerId);
    QCOMPARE(db.getOwnerStudentname(projectId), QString("infoprojowner"));
}

void ProjectTest::test_addCollaboratorAtCreation()
{
    DatabaseHandler db;
    int classId  = makeClass(db, "CollabAtClass");   QVERIFY(classId  > 0);
    int ownerId  = makeStudent(db, "collabatowner", "Owner At", classId); QVERIFY(ownerId > 0);
    int collabId = makeStudent(db, "collabatstudy", "Collab At", classId); QVERIFY(collabId > 0);
    int periodId = makePeriod(db, "CollabAtPeriod"); QVERIFY(periodId > 0);

    int projectId = makeProject(db, "CollabAtProject", ownerId, periodId, {collabId});
    QVERIFY(projectId > 0);

    const auto collabs = db.getProjectCollaborators(projectId);
    QVERIFY(std::any_of(collabs.begin(), collabs.end(),
        [collabId](const DatabaseHandler::CollaboratorInfo &c){ return c.studentId == collabId; }));
}

void ProjectTest::test_addCollaboratorStandalone()
{
    DatabaseHandler db;
    int classId  = makeClass(db, "CollabSAClass");   QVERIFY(classId  > 0);
    int ownerId  = makeStudent(db, "collabsaowner", "SA Owner", classId); QVERIFY(ownerId > 0);
    int collabId = makeStudent(db, "collabsastudy", "SA Collab", classId); QVERIFY(collabId > 0);
    int periodId = makePeriod(db, "CollabSAPeriod"); QVERIFY(periodId > 0);

    int projectId = makeProject(db, "SAProject", ownerId, periodId);
    QVERIFY(projectId > 0);
    QVERIFY(db.getProjectCollaborators(projectId).isEmpty());

    QVERIFY(db.addCollaborator(projectId, collabId, 1));
    const auto collabs = db.getProjectCollaborators(projectId);
    auto it = std::find_if(collabs.begin(), collabs.end(),
        [collabId](const DatabaseHandler::CollaboratorInfo &c){ return c.studentId == collabId; });
    QVERIFY(it != collabs.end());
    QCOMPARE(it->permissionLevel, 1);
}

void ProjectTest::test_removeCollaborator()
{
    DatabaseHandler db;
    int classId  = makeClass(db, "RemoveCollabClass");   QVERIFY(classId  > 0);
    int ownerId  = makeStudent(db, "removecollabowner", "RC Owner", classId); QVERIFY(ownerId > 0);
    int collabId = makeStudent(db, "removecollabstudy", "RC Collab", classId); QVERIFY(collabId > 0);
    int periodId = makePeriod(db, "RemoveCollabPeriod"); QVERIFY(periodId > 0);

    int projectId = makeProject(db, "RCProject", ownerId, periodId, {collabId});
    QVERIFY(projectId > 0);

    QVERIFY(db.removeCollaborator(projectId, collabId));
    const auto collabs = db.getProjectCollaborators(projectId);
    QVERIFY(std::none_of(collabs.begin(), collabs.end(),
        [collabId](const DatabaseHandler::CollaboratorInfo &c){ return c.studentId == collabId; }));
}
