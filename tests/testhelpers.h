#ifndef TESTHELPERS_H
#define TESTHELPERS_H

#include "../src/server/modules/projects/databasehandler.h"
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDebug>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────────────────
// Shared in-memory database initialisation
// Call once from the first initTestCase() executed.
// ──────────────────────────────────────────────────────────────────────────────
inline void initTestDatabase()
{
    // Guard: only open if not already open
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid() && db.isOpen())
        return;

    QSqlDatabase::addDatabase("QSQLITE");
    db = QSqlDatabase::database();
    db.setDatabaseName(":memory:");
    if (!db.open())
        qFatal("Failed to open test database: %s", qPrintable(db.lastError().text()));

    QString schemaPath = QCoreApplication::applicationDirPath() + "/../sql/schema_sqlite.sql";
    QFile schemaFile(schemaPath);
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text))
        qFatal("Failed to open schema file: %s", qPrintable(schemaPath));

    const QStringList statements =
        QString(schemaFile.readAll()).split(';', Qt::SkipEmptyParts);
    for (const QString &stmt : statements) {
        if (!stmt.trimmed().isEmpty()) {
            QSqlQuery q(db);
            if (!q.exec(stmt))
                qWarning() << "Schema statement failed:" << stmt << q.lastError();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Wipe all data between tests (child tables first)
// ──────────────────────────────────────────────────────────────────────────────
inline void cleanupDatabase()
{
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
// Factory helpers — return valid IDs or -1 / empty string on failure.
// QVERIFY the returned values in the calling test if you need early abort.
// ──────────────────────────────────────────────────────────────────────────────

inline int makeClass(DatabaseHandler &db, const QString &name, int year = 2026)
{
    if (!db.addClass(name, year, "desc")) return -1;
    return db.getClassIdByName(name);
}

inline int makeStudent(DatabaseHandler &db, const QString &uname,
                       const QString &fullName, int classId)
{
    if (!db.addStudent(uname, fullName, "pass", true, false, classId)) return -1;
    const auto students = db.getAllStudents();
    auto it = std::find_if(students.begin(), students.end(),
        [&uname](const DatabaseHandler::StudentInfo &s){ return s.studentname == uname; });
    return (it != students.end()) ? it->studentId : -1;
}

inline int makePeriod(DatabaseHandler &db, const QString &name, int year = 2026)
{
    if (!db.addPeriod(name, year, "2026-01-01", "2026-12-31")) return -1;
    const auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(),
        [&name](const DatabaseHandler::PeriodInfo &p){ return p.name == name; });
    return (it != periods.end()) ? it->periodId : -1;
}

inline int makeProject(DatabaseHandler &db, const QString &title,
                       int ownerId, int periodId,
                       const QList<int> &collaborators = {})
{
    if (!db.createEmptyProject(title, "desc", ownerId,
                               title.toLower().replace(' ', '_') + ".tup",
                               collaborators, periodId))
        return -1;
    const auto projects = db.getAllProjects();
    auto it = std::find_if(projects.begin(), projects.end(),
        [&title](const DatabaseHandler::ProjectRecord &p){ return p.title == title; });
    return (it != projects.end()) ? it->projectId : -1;
}

#endif // TESTHELPERS_H
