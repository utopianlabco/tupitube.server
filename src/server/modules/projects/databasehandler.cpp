/***************************************************************************
 *   Project TupiTube Server                                               *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Utopian Lab Development Team                                         *
 *   2010:                                                                 *
 *    Gustav Gonzalez                                                      *
 *   ---                                                                   *
 *   KTooN's versions:                                                     *
 *   2006:                                                                 *
 *    David Cuadrado                                                       *
 *    Jorge Cuadrado                                                       *
 *   2003:                                                                 *
 *    Fernado Roldan                                                       *
 *    Simena Dinas                                                         *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/
#include "databasehandler.h"
#include "tconfig.h"

#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>

DatabaseHandler::DatabaseHandler()
{
}

void DatabaseHandler::initDataBase()
{
    TCONFIG->beginGroup("Database");
    QString driver = TCONFIG->value("Driver").toString();
    db = QSqlDatabase::addDatabase(driver);

    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::initDataBase()] - Config file path:" << TCONFIG->configPath();
        qDebug() << "[DatabaseHandler::initDataBase()] - DB Drivers:" << QSqlDatabase::drivers();
    #endif

    if (driver == "QSQLITE") {
        // SQLite: Get database path and name
        QString dbDir = TCONFIG->value("DatabasePath").toString();
        QString dbName = TCONFIG->value("DbName", "tupitube.db").toString();
        QString dbPath = dbDir + "/" + dbName;

        // Create directory if it doesn't exist
        QDir dir(dbDir);
        if (!dir.exists()) {
            #ifdef TUP_DEBUG
                qDebug() << "[DatabaseHandler::initDataBase()] - Creating database directory:" << dbDir;
            #endif
            if (!dir.mkpath(".")) {
                qCritical() << "[DatabaseHandler::initDataBase()] - Failed to create database directory:" << dbDir;
                exit(1);
            }
        }

        #ifdef TUP_DEBUG
            qDebug() << "[DatabaseHandler::initDataBase()] - Using DB driver:" << driver;
            qDebug() << "[DatabaseHandler::initDataBase()] - Using DB path:" << dbPath;
        #endif
        db.setDatabaseName(dbPath);
    } else {
        // MySQL/PostgreSQL: use host, port, credentials
        db.setHostName(TCONFIG->value("Host").toString());
        db.setPort(TCONFIG->value("Port").toInt());
        db.setDatabaseName(TCONFIG->value("DbName").toString());
        db.setUserName(TCONFIG->value("Student").toString());
        db.setPassword(TCONFIG->value("Password").toString());
        if (driver == "QMYSQL")
            db.setConnectOptions("MYSQL_OPT_RECONNECT=1");
    }

    bool ok = db.open();
    if (!ok) {
        QSqlError error = db.lastError();
        #ifdef TUP_DEBUG
               qDebug() << "[DatabaseHandler::initDataBase()] - Fatal Error: Cannot connect to DB server...";
               qDebug() << "[DatabaseHandler::initDataBase()] - Description: " << error.text();
        #endif
        exit(1);
    }

    // Enforce foreign key constraints for SQLite and check schema
    if (driver == "QSQLITE") {
        QSqlQuery pragmaQuery(db);
        pragmaQuery.exec("PRAGMA foreign_keys = ON;");

        // Check that the foreign key constraint on tupitube_project.student_id is ON DELETE RESTRICT
        QSqlQuery fkQuery(db);
        fkQuery.exec("PRAGMA foreign_key_list('tupitube_project')");
        bool foundRestrict = false;
        while (fkQuery.next()) {
            QString from = fkQuery.value(3).toString(); // 'from' column
            QString to = fkQuery.value(4).toString();   // 'to' column
            QString onDelete = fkQuery.value(6).toString(); // 'on_delete' column
            if (from == "student_id" && to == "student_id" && onDelete.toUpper() == "RESTRICT") {
                foundRestrict = true;
                break;
            }
        }
        if (!foundRestrict) {
            qWarning() << "[DatabaseHandler::initDataBase()] - WARNING: Foreign key constraint on tupitube_project.student_id is not ON DELETE RESTRICT. Project ownership integrity is NOT enforced!";
        }
    }

    // Check if tables exist, create them if not
    // (Schema creation is now handled by DatabaseHandler in TupServerWindow)

    // Now that tables are created, check foreign key constraints only if the table exists
    if (driver == "QSQLITE") {
        QStringList tables = db.tables();
        if (tables.contains("tupitube_project")) {
            QSqlQuery fkQuery(db);
            fkQuery.exec("PRAGMA foreign_key_list('tupitube_project')");
            bool foundRestrict = false;
            while (fkQuery.next()) {
                QString from = fkQuery.value(3).toString(); // 'from' column
                QString to = fkQuery.value(4).toString();   // 'to' column
                QString onDelete = fkQuery.value(6).toString(); // 'on_delete' column
                if (from == "student_id" && to == "student_id" && onDelete.toUpper() == "RESTRICT") {
                    foundRestrict = true;
                    break;
                }
            }
            if (!foundRestrict) {
                qWarning() << "[DatabaseHandler::initDataBase()] - WARNING: Foreign key constraint on tupitube_project.student_id is not ON DELETE RESTRICT. Project ownership integrity is NOT enforced!";
            }
        }
    }

    TCONFIG->endGroup(); // Database
}

void DatabaseHandler::createDatabaseSchema()
{
    // QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);

    // Create class table
    QString createClassTable =
        "CREATE TABLE IF NOT EXISTS tupitube_class ("
        "class_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name VARCHAR(50) NOT NULL,"
        "year INTEGER NOT NULL,"
        "description TEXT"
        ")";
    query.exec(createClassTable);

    // Create period table
    QString createPeriodTable =
        "CREATE TABLE IF NOT EXISTS tupitube_period ("
        "period_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name VARCHAR(50) NOT NULL,"
        "year INTEGER NOT NULL,"
        "start_date DATE,"
        "end_date DATE"
        ")";
    query.exec(createPeriodTable);

    // Create tupitube_student table (updated: uses class_id)
    QString createStudentTable =
        "CREATE TABLE IF NOT EXISTS tupitube_student ("
        "student_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name VARCHAR(100),"
        "studentname VARCHAR(50) NOT NULL UNIQUE,"
        "password VARCHAR(255) NOT NULL,"
        "is_enabled INTEGER DEFAULT 1,"
        "is_creator INTEGER DEFAULT 1,"
        "projects_public_policy INTEGER DEFAULT 0,"
        "files_public_policy INTEGER DEFAULT 0,"
        "works_public_policy INTEGER DEFAULT 0,"
        "class_id INTEGER NOT NULL,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (class_id) REFERENCES tupitube_class(class_id)"
        ")";
    query.exec(createStudentTable);

    // Create tupitube_project table (updated: render tracking fields)
    QString createProjectTable =
        "CREATE TABLE IF NOT EXISTS tupitube_project ("
        "project_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title VARCHAR(100) NOT NULL,"
        "description TEXT,"
        "student_id INTEGER NOT NULL,"
        "filename VARCHAR(255) NOT NULL,"
        "is_public INTEGER DEFAULT 0,"
        "is_shared INTEGER DEFAULT 0,"
        "class_id INTEGER NOT NULL,"
        "period_id INTEGER NOT NULL,"
        "group_project INTEGER DEFAULT 0,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "last_rendered_at DATETIME,"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,"
        "FOREIGN KEY (class_id) REFERENCES tupitube_class(class_id),"
        "FOREIGN KEY (period_id) REFERENCES tupitube_period(period_id)"
        ")";
    query.exec(createProjectTable);

    // Create project_student join table
    QString createProjectStudentTable =
        "CREATE TABLE IF NOT EXISTS tupitube_project_student ("
        "project_id INTEGER NOT NULL,"
        "student_id INTEGER NOT NULL,"
        "PRIMARY KEY (project_id, student_id),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id)"
        ")";
    query.exec(createProjectStudentTable);

    // Create tupitube_chat table
    QString createChatTable =
        "CREATE TABLE IF NOT EXISTS tupitube_chat ("
        "chat_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "project_id INTEGER,"
        "student_id INTEGER NOT NULL,"
        "studentname VARCHAR(50) NOT NULL,"
        "message TEXT NOT NULL,"
        "message_type VARCHAR(20) DEFAULT 'chat',"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT"
        ");";
    query.exec(createChatTable);

    // Create tupitube_collection table
    QString createCollectionTable =
        "CREATE TABLE IF NOT EXISTS tupitube_collection ("
        "collection_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_id INTEGER,"
        "type VARCHAR(20),"
        "title VARCHAR(100),"
        "topics TEXT,"
        "description TEXT,"
        "student_id INTEGER NOT NULL,"
        "is_public INTEGER DEFAULT 0,"
        "visits INTEGER DEFAULT 0,"
        "likes INTEGER DEFAULT 0,"
        "project_id INTEGER,"
        "path VARCHAR(255),"
        "slug VARCHAR(100),"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (parent_id) REFERENCES tupitube_collection(collection_id)"
        ")";
    query.exec(createCollectionTable);

    // Create tupitube_work table
    QString createWorkTable =
        "CREATE TABLE IF NOT EXISTS tupitube_work ("
        "work_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "project_id INTEGER,"
        "collection_id INTEGER,"
        "type_id INTEGER,"
        "type VARCHAR(20),"
        "title VARCHAR(100),"
        "content TEXT,"
        "topics TEXT,"
        "tags TEXT,"
        "description TEXT,"
        "student_id INTEGER,"
        "filename VARCHAR(255),"
        "is_public INTEGER DEFAULT 0,"
        "enabled INTEGER DEFAULT 1,"
        "visits INTEGER DEFAULT 0,"
        "duration INTEGER DEFAULT 0,"
        "portrait INTEGER DEFAULT 0,"
        "mobile INTEGER DEFAULT 0,"
        "rendered INTEGER DEFAULT 0,"
        "uploaded INTEGER DEFAULT 0,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (collection_id) REFERENCES tupitube_collection(collection_id),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id)"
        ")";
    query.exec(createWorkTable);

    // Create tupitube_collaboration table
    QString createCollaborationTable =
        "CREATE TABLE IF NOT EXISTS tupitube_collaboration ("
        "collaboration_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "project_id INTEGER NOT NULL,"
        "student_id INTEGER NOT NULL,"
        "permission_level INTEGER DEFAULT 1,"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id)"
        ")";
    query.exec(createCollaborationTable);

    // Create tupitube_grade table
    QString createGradeTable =
        "CREATE TABLE IF NOT EXISTS tupitube_grade ("
        "grade_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "project_id INTEGER NOT NULL,"
        "student_id INTEGER NOT NULL,"
        "teacher_student_id INTEGER NOT NULL DEFAULT 0,"
        "period_id INTEGER NOT NULL,"
        "class_id INTEGER NOT NULL,"
        "grade TEXT NOT NULL,"
        "comments TEXT,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "UNIQUE(project_id, student_id, teacher_student_id, period_id, class_id),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id)"
        ")";
    query.exec(createGradeTable);
}

DatabaseHandler::~DatabaseHandler()
{
}

QString DatabaseHandler::incomingFolderID(const QString &uid, const QString &type) const
{
    QString folderID = "";
    QString typeKey = type.toLower();
    QString sql = "SELECT collection_id FROM tupitube_collection WHERE student_id=" + uid
            + " AND title='Incoming " + type + "' AND type='" + typeKey + "'";
    QSqlQuery query(sql);

    if (query.next() && query.first())
        folderID = query.value(0).toString();

    query.clear();

    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::incomingFolderID()] - SQL: " << sql;
    #endif

    return folderID;
}

QString DatabaseHandler::worksPublicPolicy(const QString &owner) const
{
    if (owner.compare("anonymous") == 0) 
        return "true";

    QString isPublic = "false";
    QString sql = "SELECT works_public_policy FROM tupitube_student WHERE student_id=" + owner;
    QSqlQuery query(sql);
    if (query.next() && query.first())
        isPublic = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::worksPublicPolicy()] - SQL: " << sql;
    #endif

    return isPublic;
}

QString DatabaseHandler::projectKey(const QString &filename) const
{
    QString projectID = "";
    QString sql = "SELECT project_id FROM tupitube_project WHERE filename='" + filename + "'";
    QSqlQuery query(sql);
    if (query.next() && query.first())
        projectID = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::projectKey()] - SQL: " << sql;
    #endif

    return projectID;
}

bool DatabaseHandler::addProject(const NetProject *project)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addProject()]";
    #endif

    QString owner = QString::number(project->owner());
    QString isPublic = "false";
    QString isShared = "false";

    QString sql = "SELECT files_public_policy, projects_public_policy FROM tupitube_student WHERE student_id=" + owner;
    QSqlQuery query(sql);
    if (query.next() && query.first()) {
        isPublic = query.value(0).toString();
        isShared = query.value(1).toString();
    }
    query.clear();

    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addProject()] - SQL: " << sql;
    #endif

    // New fields for schema compliance
    int classId = project->property("class_id").toInt();
    int periodId = project->property("period_id").toInt();
    bool groupProject = project->property("group_project").toBool();
    QList<int> studentIds = project->property("student_ids").value<QList<int>>();

    QString name = project->getName().replace("'", "\\'");
    QString description = project->getDescription().replace("'", "\\'");

    sql = "INSERT INTO tupitube_project (title, description, student_id, filename, is_public, is_shared, class_id, period_id, group_project, created_at, updated_at) VALUES(";
    sql += "'" + name + "', ";
    sql += "'" + description + "', ";
    sql += owner + ", ";
    sql += "'" + project->filename() + "', ";
    sql += isPublic + ", ";
    sql += isShared + ", ";
    sql += QString::number(classId) + ", ";
    sql += QString::number(periodId) + ", ";
    sql += (groupProject ? "1" : "0") + QString(", ");
    sql += "datetime('now'), ";
    sql += "datetime('now'))";

    #ifdef TUP_DEBUG
        qDebug() << "DatabaseHandler::addProject() SQL: " << sql;
    #endif

    bool isOk = query.exec(sql);
    if (!isOk) return false;

    // If group project, insert into project_student
    if (groupProject && !studentIds.isEmpty()) {
        // Get the last inserted project_id
        int projectId = -1;
        QSqlQuery lastIdQuery("SELECT last_insert_rowid()");
        if (lastIdQuery.next()) {
            projectId = lastIdQuery.value(0).toInt();
        }
        for (int studentId : studentIds) {
            QSqlQuery psQuery;
            QString psSql = QString("INSERT INTO tupitube_project_student (project_id, student_id) VALUES (%1, %2)").arg(projectId).arg(studentId);
            psQuery.exec(psSql);
        }
    }
    return true;
}

QString DatabaseHandler::getStudentID(const QString &studentname) const
{
    QString uid = "1";

    if (studentname.compare("anonymous") != 0) {
        QSqlQuery query;
        query.exec("SELECT id FROM student WHERE studentname='" + studentname + "'");
        if (query.next()) {
            uid = query.value(0).toString();
            qDebug() << "[DatabaseHandler::getStudentID()] - UID: " <<  uid;
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[DatabaseHandler::getStudentID()] - Fatal Error: Invalid studentname -> " << studentname;
            #endif
        }
    }

    return uid;
}

bool DatabaseHandler::addWork(const QString &projectID, const QString &type, const QString &owner, const QString &title, 
                              const QString &topics, const QString &desc, const QString &filename, bool portrait)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addWork()]";
        qDebug() << "[DatabaseHandler::addWork()] - projectID: " << projectID;
        qDebug() << "[DatabaseHandler::addWork()] - owner: " << owner;
    #endif

    QString isPortrait = (portrait) ? "TRUE" : "FALSE";

    QString collectionID = "";
    if (type.compare("animation") == 0) {
        collectionID = incomingFolderID(owner, "Animations");
    } else if (type.compare("image") == 0) {
        collectionID = incomingFolderID(owner, "Images");
    }

    QString isPublic = worksPublicPolicy(owner);

    QString workTitle = title;
    QString workTopics = topics;
    QString workDescription = desc;

    // Updated for new schema: type_id, tags, enabled, visits, duration, mobile, rendered, uploaded
    QString sql = "INSERT INTO tupitube_work (project_id, collection_id, type_id, type, title, content, topics, tags, description, student_id, filename, is_public, enabled, visits, duration, portrait, mobile, rendered, uploaded, created_at, updated_at) VALUES(";
    sql += projectKey(projectID) + ", ";
    sql += collectionID + ", ";
    sql += "NULL, "; // type_id (not provided)
    sql += "'" + type + "', ";
    sql += "'" + workTitle.replace("'", "''") + "', ";
    sql += "NULL, "; // content (not provided)
    sql += "'" + workTopics.replace("'", "") + "', ";
    sql += "'', "; // tags (not provided)
    sql += "'" + workDescription.replace("'", "''") + "', ";
    sql += owner + ", ";
    sql += "'" + filename + "', ";
    sql += isPublic + ", ";
    sql += "1, "; // enabled
    sql += "0, "; // visits
    sql += "0, "; // duration
    sql += isPortrait + ", ";
    sql += "0, 0, 0, "; // mobile, rendered, uploaded
    sql += "datetime('now'), ";
    sql += "datetime('now'))";

    QSqlQuery query;
    bool isOk = query.exec(sql);

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::addWork()] - SQL: " << sql;
    #endif

    return isOk;
}

bool DatabaseHandler::addStoryFrame(const QString &storyboard, const QString &id, const QString &owner, const QString &title, 
                                    const QString &topics, const QString &description, const QString &filename, const QString &duration)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addStoryFrame()]";
    #endif

    QString projectID = projectKey(id);

    if (projectID.length() == 0) {
        #ifdef TUP_DEBUG
               qDebug() << "[DatabaseHandler::addStoryFrame()] - Fatal Error: project_id key couldn't be found!";
        #endif
        return false;
    }

        QString isPublic = worksPublicPolicy(owner);

        QString workTitle = title;
        QString workTopics = topics;
        QString workDescription = description;

        // Updated for new schema
        QString sql = "INSERT INTO tupitube_work (project_id, collection_id, type_id, type, title, content, topics, tags, description, student_id, filename, is_public, enabled, visits, duration, portrait, mobile, rendered, uploaded, created_at, updated_at) VALUES(";
        sql += projectID + ", ";
        sql += storyboard + ", ";
        sql += "NULL, "; // type_id
        sql += "'frame', ";
        sql += "'" + workTitle.replace("'", "\\'") + "', ";
        sql += "NULL, "; // content
        sql += "'" + workTopics.replace("'", "") + "', ";
        sql += "'', "; // tags
        sql += "'" + workDescription.replace("'", "\\'") + "', ";
        sql += owner + ", ";
        sql += "'" + filename + "', ";
        sql += isPublic + ", ";
        sql += "1, "; // enabled
        sql += "0, "; // visits
        sql += "'" + duration + "', ";
        sql += "0, 0, 0, 0, "; // portrait, mobile, rendered, uploaded
        sql += "datetime('now'), ";
        sql += "datetime('now'))";

        QSqlQuery query;
        bool isOk = query.exec(sql);

        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::addStoryFrame()] - SQL: " << sql;
        #endif

        return isOk;
}

bool DatabaseHandler::addStoryboard(const QString &id, const QString &owner, const QString &title, const QString &topics, const QString &description, const QString &path)
{
    QString projectID = projectKey(id);

    if (projectID.length() == 0) {
        #ifdef TUP_DEBUG
               qDebug() << "[DatabaseHandler::addStoryboard()] - Fatal Error: project_id key couldn't be found!";
        #endif
        return false;
    }

    QString incomingFolder = incomingFolderID(owner, "Storyboards");

    if (incomingFolder.length() == 0) {
        #ifdef TUP_DEBUG
               qDebug() << "[DatabaseHandler::addStoryboard()] - Fatal Error: Storyboard incoming folder doesn't exist for student " << owner;
        #endif
        return false;
    }

    QString isPublic = worksPublicPolicy(owner);
    QString slug = title.toLower();
    slug.replace(QString(" "), QString("-"));
    int i = 0;
    while (true) {
           if (slugExists(slug, owner)) {
               slug += "-" + QString::number(i);
               i++;
           } else {
               break;
           }
    }

    QString workTitle = title;
    QString workTopics = topics;
    QString workDescription = description;

        QString sql = "INSERT INTO tupitube_collection (parent_id, type, title, topics, description, student_id, is_public, visits, likes, project_id, path, created_at, updated_at, slug) VALUES(";
        sql += incomingFolder + ", ";
        sql += "'story', ";
        sql += "'" + workTitle.replace("'", "\\'") + "', ";
        sql += "'" + workTopics.replace("'", "") + "', ";
        sql += "'" + workDescription.replace("'", "\\'") + "', ";
        sql += owner + ", ";
        sql += isPublic + ", ";
        sql += "0, 0, ";
        sql += projectID + ", ";
        sql += "'" + path + "', ";
        sql += "datetime('now'), ";
        sql += "datetime('now'), ";
        sql += "'" +  slug + "')";

        QSqlQuery query;
        bool isOk = query.exec(sql);

        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::addStoryboard()] - SQL: " << sql;
        #endif

        return isOk;
}

bool DatabaseHandler::slugExists(const QString &slug, const QString &owner)
{
    QString sql = "SELECT count(*) FROM tupitube_collection WHERE slug='" + slug + "' AND student_id=" + owner;

    int count = -1; 
    QSqlQuery query = QSqlQuery(sql);
    if (query.next() && query.first())
        count = query.value(0).toInt();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::slugExists()] - SQL: " << sql;
    #endif

    if (count == 0)
        return false;

    return true;
}

QString DatabaseHandler::storyboardID(const QString &uid, const QString &directory) const
{
    QString id = "-1";
    QString sql = "SELECT collection_id FROM tupitube_collection WHERE student_id=" + uid + " AND path ='" + directory + "'";

    QSqlQuery query = QSqlQuery(sql);
    if (query.next() && query.first())
        id = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::storyboardID()] - SQL: " << sql;
    #endif

    return id;
}

QList< DatabaseHandler::ProjectInfo> DatabaseHandler::studentProjects(int studentID, const QString &login)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::studentProjects()]";
    #endif

    QList<DatabaseHandler::ProjectInfo> list;
    QString sql = "SELECT p.title, p.description, p.filename, p.created_at, p.class_id, c.name, p.period_id, per.name, p.group_project "
                  "FROM tupitube_project p "
                  "LEFT JOIN tupitube_class c ON p.class_id = c.class_id "
                  "LEFT JOIN tupitube_period per ON p.period_id = per.period_id "
                  "WHERE p.student_id=" + QString::number(studentID) + " ORDER BY p.created_at DESC";
    QSqlQuery query(sql);
    while (query.next()) {
        DatabaseHandler::ProjectInfo record;
        record.title = query.value(0).toString();
        record.owner = login;
        record.description = query.value(1).toString();
        record.file = query.value(2).toString();
        QDateTime date = query.value(3).toDateTime();
        record.date = date.toString("dd/MM/yyyy hh:mm");
        record.classId = query.value(4).toInt();
        record.className = query.value(5).toString();
        record.periodId = query.value(6).toInt();
        record.periodName = query.value(7).toString();
        record.groupProject = query.value(8).toBool();
        list.append(record);
    }
    query.clear();
    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::studentProjects()] - SQL: " << sql;
    #endif
    return list;
}

QList< DatabaseHandler::ProjectInfo> DatabaseHandler::partnerProjects(int studentID)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::partnerProjects()]";
    #endif

    QList<DatabaseHandler::ProjectInfo> list;
    QList<QString> projects;

    QString sql = "SELECT project_id FROM tupitube_collaboration WHERE student_id=" + QString::number(studentID);
    QSqlQuery query(sql);
    while (query.next()) {
           QString projectID = query.value(0).toString();
           projects.append(projectID);
    }
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::partnerProjects()] - SQL: " << sql;
    #endif

    for (int i=0; i < projects.size(); i++) {
         QSqlQuery query("SELECT title, description, student_id, filename, created_at FROM tupitube_project WHERE project_id=" + projects.at(i));
         QString name = "";
         QString description = "";
         QString date = "";

         while (query.next()) {
                DatabaseHandler::ProjectInfo record;
                record.title = query.value(0).toString();
                record.description = query.value(1).toString();
                QString owner = query.value(2).toString();
                record.owner = studentLogin(owner);
                record.file = query.value(3).toString();
                QDateTime date = query.value(4).toDateTime();
                record.date = date.toString("dd/MM/yyyy hh:mm"); 

                list.append(record);
         }
         query.clear();

         #ifdef TUP_DEBUG
                  qWarning() << "[DatabaseHandler::partnerProjects()] - SQL: " << sql;
         #endif
    }

    return list;
}

bool DatabaseHandler::accessIsConfirmed(const QString &projectID, int studentID)
{
    QString uid = QString::number(studentID);

    QString sql = "SELECT count(*) FROM tupitube_project WHERE project_id=" + projectID + " AND student_id=" + uid;
    QSqlQuery query(sql);

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::accessIsConfirmed()] - SQL: " << sql;
    #endif

    if (query.first()) {
        int count = query.value(0).toInt();
        query.clear();
        if (count == 1)
            return true;
    }

    sql = "SELECT count(*) FROM tupitube_collaboration WHERE student_id=" + uid + " AND project_id=" + projectID;
    query = QSqlQuery(sql);
    if (query.first()) {
        int count = query.value(0).toInt();
        query.clear();
        #ifdef TUP_DEBUG
               qWarning() << "[DatabaseHandler::accessIsConfirmed()] - SQL: " << sql;
        #endif
        if (count == 1)
            return true;
    }

    return false;
}

QString DatabaseHandler::studentLogin(const QString &owner) const
{
    QString login = "unknown";
 
    QString sql = "SELECT studentname FROM tupitube_student WHERE student_id=" + owner;
    QSqlQuery query(sql);
    if (query.first())
        login = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::studentLogin()] - SQL: " << sql;
    #endif

    return login;
}

QString DatabaseHandler::studentID(const QString &login) const
{
    QString id = "unknown";

    QString sql = "SELECT student_id FROM tupitube_student WHERE studentname='" + login + "'";
    QSqlQuery query(sql);
    if (query.first())
        id = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::studentID()] - SQL: " << sql;
    #endif

    return id;
}

QString DatabaseHandler::exists(const QString &filename, const QString &ownerID) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::exists()]";
    #endif

    QString sql = "SELECT project_id FROM tupitube_project WHERE filename='" + filename + "' AND student_id=" + ownerID;
    QSqlQuery query(sql);
    QString id = "";
    if (query.first())
        id = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::exists()] - SQL: " << sql;
    #endif

    if (id.length() > 0)
        return id;

    return "-1"; 
}

int DatabaseHandler::count(const QString &sql)
{
    QSqlQuery query(sql);
    int total = 0;
    if (query.first())
        total = query.value(0).toInt();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::count()] - SQL: " << sql;
    #endif

    return total;
}

// This method shouldn't be necessary
bool DatabaseHandler::addLog(const QString &type, const QString &filename, const QString &ip)
{
    QSqlQuery query;
    QString sql = "";

    sql = "INSERT INTO tupitube_log (type, filename, ip, date) VALUES(";
    sql += "'" + type + "', ";
    sql += "'" + filename + "', ";
    sql += "'" + ip + "', ";
    sql += "datetime('now'))";

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::addLog()] - SQL: " << sql;
    #endif

    bool isOk = query.exec(sql);

    return isOk;
}

QList<DatabaseHandler::StudentInfo> DatabaseHandler::getAllStudents() const
{
    QList<StudentInfo> students;
    QSqlQuery query("SELECT u.student_id, u.name, u.class_id, c.name as class_name, u.studentname, u.password, u.is_enabled, u.is_creator FROM tupitube_student u LEFT JOIN tupitube_class c ON u.class_id = c.class_id ORDER BY u.studentname");
    while (query.next()) {
        StudentInfo student;
        student.studentId    = query.value(0).toInt();
        student.name      = query.value(1).toString();
        student.classId   = query.value(2).toInt();
        student.className = query.value(3).toString();
        student.studentname  = query.value(4).toString();
        student.password  = query.value(5).toString();
        student.isEnabled = query.value(6).toBool();
        student.isCreator = query.value(7).toBool();
        students.append(student);
    }
    return students;
}

bool DatabaseHandler::addStudent(const QString &studentname, const QString &name, const QString &password, bool isEnabled, bool isCreator, int classId)
{
    if (studentnameExists(studentname))
        return false;

    QSqlQuery query;
    query.prepare("INSERT INTO tupitube_student (studentname, name, password, is_enabled, is_creator, class_id) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(studentname);
    query.addBindValue(name);
    query.addBindValue(password);
    query.addBindValue(isEnabled ? 1 : 0);
    query.addBindValue(isCreator ? 1 : 0);
    query.addBindValue(classId);
    return query.exec();
}

bool DatabaseHandler::updateStudent(int studentId, const QString &studentname, const QString &name, const QString &password, bool isEnabled, bool isCreator, int classId)
{
    QSqlQuery query;
    if (password.isEmpty()) {
        query.prepare("UPDATE tupitube_student SET studentname=?, name=?, is_enabled=?, is_creator=?, class_id=?, updated_at=datetime('now') WHERE student_id=?");
        query.addBindValue(studentname);
        query.addBindValue(name);
        query.addBindValue(isEnabled ? 1 : 0);
        query.addBindValue(isCreator ? 1 : 0);
        query.addBindValue(classId);
        query.addBindValue(studentId);
    } else {
        query.prepare("UPDATE tupitube_student SET studentname=?, name=?, password=?, is_enabled=?, is_creator=?, class_id=?, updated_at=datetime('now') WHERE student_id=?");
        query.addBindValue(studentname);
        query.addBindValue(name);
        query.addBindValue(password);
        query.addBindValue(isEnabled ? 1 : 0);
        query.addBindValue(isCreator ? 1 : 0);
        query.addBindValue(classId);
        query.addBindValue(studentId);
    }
    return query.exec();
}

bool DatabaseHandler::removeStudent(int studentId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tupitube_student WHERE student_id = ?");
    query.addBindValue(studentId);
    bool isOk = query.exec();
    if (!isOk) {
        QSqlError err = query.lastError();
        if (err.type() == QSqlError::StatementError && err.nativeErrorCode() == "19") {
            qWarning() << "[DatabaseHandler::removeStudent()] - Cannot delete student: owns projects or is referenced.";
        } else {
            qWarning() << "[DatabaseHandler::removeStudent()] - Error:" << err.text();
        }
    }
    return isOk;
}

int DatabaseHandler::getClassIdByName(const QString &name) const
{
    QSqlQuery query;
    query.prepare("SELECT class_id FROM tupitube_class WHERE name = ?");
    query.addBindValue(name);
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return -1;
}

bool DatabaseHandler::studentnameExists(const QString &studentname) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::studentnameExists()] - studentname:" << studentname;
    #endif

    QString sql = "SELECT COUNT(*) FROM tupitube_student WHERE studentname = '" + studentname + "'";
    QSqlQuery query(sql);
    
    if (query.first()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

// Collaboration management methods

QList<DatabaseHandler::ProjectRecord> DatabaseHandler::getAllProjects() const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::getAllProjects()]";
    #endif

    QList<ProjectRecord> projects;
    QString sql = "SELECT p.project_id, p.title, p.filename, p.student_id, u.studentname, p.description, "
                  "p.created_at, p.is_shared, p.class_id, c.name, p.period_id, per.name, p.group_project, "
                  "p.last_rendered_at, p.updated_at "
                  "FROM tupitube_project p "
                  "LEFT JOIN tupitube_student u ON p.student_id = u.student_id "
                  "LEFT JOIN tupitube_class c ON p.class_id = c.class_id "
                  "LEFT JOIN tupitube_period per ON p.period_id = per.period_id "
                  "ORDER BY p.created_at DESC";
    QSqlQuery query(sql);
    while (query.next()) {
        ProjectRecord record;
        record.projectId = query.value(0).toInt();
        record.title = query.value(1).toString();
        record.filename = query.value(2).toString();
        record.ownerId = query.value(3).toInt();
        record.ownerStudentname = query.value(4).toString();
        record.description = query.value(5).toString();
        record.createdAt = query.value(6).toString();
        record.isShared = query.value(7).toBool();
        record.classId = query.value(8).toInt();
        record.className = query.value(9).toString();
        record.periodId = query.value(10).toInt();
        record.periodName = query.value(11).toString();
        record.groupProject = query.value(12).toBool();
        record.lastRenderedAt = query.value(13).toString();
        record.updatedAt = query.value(14).toString();
        projects.append(record);
    }
    query.clear();
    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::getAllProjects()] - Found" << projects.size() << "projects";
    #endif
    return projects;
}

QList<DatabaseHandler::CollaboratorInfo> DatabaseHandler::getProjectCollaborators(int projectId) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::getProjectCollaborators()] - projectId:" << projectId;
    #endif

    QList<CollaboratorInfo> collaborators;
    QString sql = "SELECT c.collaboration_id, c.student_id, u.studentname, u.name, c.permission_level "
                  "FROM tupitube_collaboration c "
                  "LEFT JOIN tupitube_student u ON c.student_id = u.student_id "
                  "WHERE c.project_id = " + QString::number(projectId) + " "
                  "ORDER BY u.studentname";
    QSqlQuery query(sql);

    while (query.next()) {
        CollaboratorInfo info;
        info.collaborationId = query.value(0).toInt();
        info.studentId = query.value(1).toInt();
        info.studentname = query.value(2).toString();
        info.name = query.value(3).toString();
        info.permissionLevel = query.value(4).toInt();
        collaborators.append(info);
    }

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::getProjectCollaborators()] - Found" << collaborators.size() << "collaborators";
    #endif

    return collaborators;
}

bool DatabaseHandler::addCollaborator(int projectId, int studentId, int permissionLevel)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addCollaborator()] - projectId:" << projectId << "studentId:" << studentId;
    #endif

    // Check if collaboration already exists
    QString checkSql = "SELECT COUNT(*) FROM tupitube_collaboration WHERE project_id = " 
                       + QString::number(projectId) + " AND student_id = " + QString::number(studentId);
    QSqlQuery checkQuery(checkSql);
    if (checkQuery.first() && checkQuery.value(0).toInt() > 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[DatabaseHandler::addCollaborator()] - Collaboration already exists";
        #endif
        return false;
    }

    QString sql = "INSERT INTO tupitube_collaboration (project_id, student_id, permission_level) VALUES (";
    sql += QString::number(projectId) + ", ";
    sql += QString::number(studentId) + ", ";
    sql += QString::number(permissionLevel) + ")";

    QSqlQuery query;
    bool isOk = query.exec(sql);

    if (isOk) {
        // Mark project as shared
        QString updateSql = "UPDATE tupitube_project SET is_shared = 1 WHERE project_id = " + QString::number(projectId);
        QSqlQuery updateQuery;
        updateQuery.exec(updateSql);
    }

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::addCollaborator()] - SQL:" << sql;
        if (!isOk)
            qWarning() << "[DatabaseHandler::addCollaborator()] - Error:" << query.lastError().text();
    #endif

    return isOk;
}

bool DatabaseHandler::removeCollaborator(int projectId, int studentId)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::removeCollaborator()] - projectId:" << projectId << "studentId:" << studentId;
    #endif

    QString sql = "DELETE FROM tupitube_collaboration WHERE project_id = " + QString::number(projectId) 
                  + " AND student_id = " + QString::number(studentId);

    QSqlQuery query;
    bool isOk = query.exec(sql);

    if (isOk) {
        // Check if project still has collaborators
        QString checkSql = "SELECT COUNT(*) FROM tupitube_collaboration WHERE project_id = " + QString::number(projectId);
        QSqlQuery checkQuery(checkSql);
        if (checkQuery.first() && checkQuery.value(0).toInt() == 0) {
            // No more collaborators, mark project as not shared
            QString updateSql = "UPDATE tupitube_project SET is_shared = 0 WHERE project_id = " + QString::number(projectId);
            QSqlQuery updateQuery;
            updateQuery.exec(updateSql);
        }
    }

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::removeCollaborator()] - SQL:" << sql;
        if (!isOk)
            qWarning() << "[DatabaseHandler::removeCollaborator()] - Error:" << query.lastError().text();
    #endif

    return isOk;
}

bool DatabaseHandler::createEmptyProject(const QString &title, const QString &description, int ownerId, 
                                          const QString &filename, const QList<int> &collaboratorIds, int periodId)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::createEmptyProject()] - title:" << title << "owner:" << ownerId;
    #endif

    bool isShared = !collaboratorIds.isEmpty();

    // Look up the class_id for the owner student
    int classId = -1;
    {
        QSqlQuery classQuery;
        classQuery.prepare("SELECT class_id FROM tupitube_student WHERE student_id = ?");
        classQuery.addBindValue(ownerId);
        if (classQuery.exec() && classQuery.next()) {
            classId = classQuery.value(0).toInt();
        }
    }
    if (classId == -1) {
        #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::createEmptyProject()] - Could not find class_id for owner student:" << ownerId;
        #endif
        return false;
    }
    int groupProject = isShared ? 1 : 0;

    QString sql = "INSERT INTO tupitube_project (title, description, student_id, filename, is_shared, class_id, period_id, group_project) VALUES (";
    sql += "'" + title + "', ";
    sql += "'" + description + "', ";
    sql += QString::number(ownerId) + ", ";
    sql += "'" + filename + "', ";
    sql += QString::number(isShared ? 1 : 0) + ", ";
    sql += QString::number(classId) + ", ";
    sql += QString::number(periodId) + ", ";
    sql += QString::number(groupProject) + ")";
    QSqlQuery query;
    bool isOk = query.exec(sql);

    if (!isOk) {
        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::createEmptyProject()] - Error creating project:" << query.lastError().text();
        #endif
        return false;
    }

    // Get the newly created project ID
    int projectId = query.lastInsertId().toInt();

    // Add collaborators
    for (int studentId : collaboratorIds) {
        if (studentId != ownerId) {  // Don't add owner as collaborator
            addCollaborator(projectId, studentId);
        }
    }

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::createEmptyProject()] - Created project ID:" << projectId;
    #endif

    return true;
}
QString DatabaseHandler::getProjectFilename(int projectId) const
{
    QSqlQuery query;
    query.exec("SELECT filename FROM tupitube_project WHERE project_id = " + QString::number(projectId));
    if (query.next())
        return query.value(0).toString();
    return QString();
}

int DatabaseHandler::getProjectOwnerId(int projectId) const
{
    QSqlQuery query;
    query.exec("SELECT student_id FROM tupitube_project WHERE project_id = " + QString::number(projectId));
    if (query.next())
        return query.value(0).toInt();
    return -1;
}

QString DatabaseHandler::getOwnerStudentname(int projectId) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::getOwnerStudentname()] - projectId:" << projectId;
    #endif

    QString sql = "SELECT u.studentname FROM tupitube_project p "
                  "LEFT JOIN tupitube_student u ON p.student_id = u.student_id "
                  "WHERE p.project_id = " + QString::number(projectId);
    QSqlQuery query(sql);
    if (query.next())
        return query.value(0).toString();
    return QString();
}

bool DatabaseHandler::deleteProject(int projectId)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::deleteProject()] - Deleting project ID:" << projectId;
    #endif

    QSqlDatabase::database().transaction();

    // First delete all collaborators for this project
    QSqlQuery collabQuery;
    collabQuery.exec("DELETE FROM tupitube_collaboration WHERE project_id = " + QString::number(projectId));

    // Then delete the project itself
    QSqlQuery projectQuery;
    bool success = projectQuery.exec("DELETE FROM tupitube_project WHERE project_id = " + QString::number(projectId));

    if (success && projectQuery.numRowsAffected() > 0) {
        QSqlDatabase::database().commit();
        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::deleteProject()] - Project deleted successfully";
        #endif
        return true;
    } else {
        QSqlDatabase::database().rollback();
        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::deleteProject()] - Failed to delete project:" << projectQuery.lastError().text();
        #endif
        return false;
    }
}

bool DatabaseHandler::saveChatMessage(int projectId, int studentId, const QString &studentname, 
                                      const QString &message, const QString &messageType)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::saveChatMessage()] - Student:" << studentname << "Type:" << messageType;
    #endif

    QSqlQuery query;
    query.prepare("INSERT INTO tupitube_chat (project_id, student_id, studentname, message, message_type) "
                  "VALUES (:projectId, :studentId, :studentname, :message, :messageType)");
    
    if (projectId > 0)
        query.bindValue(":projectId", projectId);
    else
        query.bindValue(":projectId", QVariant(QVariant::Int));  // NULL for global chat
    
    query.bindValue(":studentId", studentId);
    query.bindValue(":studentname", studentname);
    query.bindValue(":message", message);
    query.bindValue(":messageType", messageType);

    if (!query.exec()) {
        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::saveChatMessage()] - Error:" << query.lastError().text();
        #endif
        return false;
    }

    return true;
}

QList<DatabaseHandler::ChatMessage> DatabaseHandler::getChatHistory(int projectId, int limit) const
{
    QList<ChatMessage> messages;

    QString sql = "SELECT chat_id, project_id, student_id, studentname, message, message_type, created_at "
                  "FROM tupitube_chat ";
    
    if (projectId > 0)
        sql += "WHERE project_id = " + QString::number(projectId) + " ";
    
    sql += "ORDER BY created_at DESC LIMIT " + QString::number(limit);

    QSqlQuery query;
    query.exec(sql);

    while (query.next()) {
        ChatMessage msg;
        msg.chatId = query.value(0).toInt();
        msg.projectId = query.value(1).toInt();
        msg.studentId = query.value(2).toInt();
        msg.studentname = query.value(3).toString();
        msg.message = query.value(4).toString();
        msg.messageType = query.value(5).toString();
        msg.createdAt = query.value(6).toString();
        messages.append(msg);
    }

    return messages;
}

QList<DatabaseHandler::ChatMessage> DatabaseHandler::getChatHistoryByDate(const QString &fromDate, const QString &toDate) const
{
    QList<ChatMessage> messages;

    QSqlQuery query;
    query.prepare("SELECT chat_id, project_id, student_id, studentname, message, message_type, created_at "
                  "FROM tupitube_chat "
                  "WHERE created_at >= :fromDate AND created_at <= :toDate "
                  "ORDER BY created_at DESC");
    query.bindValue(":fromDate", fromDate);
    query.bindValue(":toDate", toDate);
    query.exec();

    while (query.next()) {
        ChatMessage msg;
        msg.chatId = query.value(0).toInt();
        msg.projectId = query.value(1).toInt();
        msg.studentId = query.value(2).toInt();
        msg.studentname = query.value(3).toString();
        msg.message = query.value(4).toString();
        msg.messageType = query.value(5).toString();
        msg.createdAt = query.value(6).toString();
        messages.append(msg);
    }

    return messages;
}

bool DatabaseHandler::clearChatHistory(int projectId)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::clearChatHistory()] - Project ID:" << projectId;
    #endif

    QSqlQuery query;
    QString sql;
    
    if (projectId > 0)
        sql = "DELETE FROM tupitube_chat WHERE project_id = " + QString::number(projectId);
    else
        sql = "DELETE FROM tupitube_chat";

    if (!query.exec(sql)) {
        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::clearChatHistory()] - Error:" << query.lastError().text();
        #endif
        return false;
    }

    return true;
}

// === Class and Period Management ===
QList<DatabaseHandler::ClassInfo> DatabaseHandler::getAllClasses() const {
    QList<ClassInfo> list;
    QSqlQuery query("SELECT class_id, name, year, description FROM tupitube_class ORDER BY year DESC, name ASC");
    while (query.next()) {
        ClassInfo c;
        c.classId = query.value(0).toInt();
        c.name = query.value(1).toString();
        c.year = query.value(2).toInt();
        c.description = query.value(3).toString();
        list.append(c);
    }
    return list;
}

bool DatabaseHandler::addClass(const QString &name, int year, const QString &description) {
    QSqlQuery query;
    query.prepare("INSERT INTO tupitube_class (name, year, description) VALUES (?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(year);
    query.addBindValue(description);
    return query.exec();
}

bool DatabaseHandler::updateClass(int classId, const QString &name, int year, const QString &description) {
    QSqlQuery query;
    query.prepare("UPDATE tupitube_class SET name=?, year=?, description=? WHERE class_id=?");
    query.addBindValue(name);
    query.addBindValue(year);
    query.addBindValue(description);
    query.addBindValue(classId);
    return query.exec();
}

bool DatabaseHandler::removeClass(int classId) {
    QSqlQuery query;
    query.prepare("DELETE FROM tupitube_class WHERE class_id=?");
    query.addBindValue(classId);
    return query.exec();
}

bool DatabaseHandler::classHasStudents(int classId) const {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM tupitube_student WHERE class_id = ?");
    query.addBindValue(classId);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    return false;
}

bool DatabaseHandler::classHasProjects(int classId) const {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM tupitube_project WHERE class_id = ?");
    query.addBindValue(classId);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    return false;
}

int DatabaseHandler::getStudentCountForClass(int classId) const {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM tupitube_student WHERE class_id = ?");
    query.addBindValue(classId);
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return 0;
}

QList<DatabaseHandler::PeriodInfo> DatabaseHandler::getAllPeriods() const {
    QList<PeriodInfo> list;
    QSqlQuery query("SELECT period_id, name, year, start_date, end_date FROM tupitube_period ORDER BY year DESC, name ASC");
    while (query.next()) {
        PeriodInfo p;
        p.periodId = query.value(0).toInt();
        p.name = query.value(1).toString();
        p.year = query.value(2).toInt();
        p.startDate = query.value(3).toString();
        p.endDate = query.value(4).toString();
        list.append(p);
    }
    return list;
}

bool DatabaseHandler::addPeriod(const QString &name, int year, const QString &startDate, const QString &endDate) {
    QSqlQuery query;
    query.prepare("INSERT INTO tupitube_period (name, year, start_date, end_date) VALUES (?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(year);
    query.addBindValue(startDate);
    query.addBindValue(endDate);
    return query.exec();
}

bool DatabaseHandler::updatePeriod(int periodId, const QString &name, int year, const QString &startDate, const QString &endDate) {
    QSqlQuery query;
    query.prepare("UPDATE tupitube_period SET name=?, year=?, start_date=?, end_date=? WHERE period_id=?");
    query.addBindValue(name);
    query.addBindValue(year);
    query.addBindValue(startDate);
    query.addBindValue(endDate);
    query.addBindValue(periodId);
    return query.exec();
}

bool DatabaseHandler::removePeriod(int periodId) {
    QSqlQuery query;
    query.prepare("DELETE FROM tupitube_period WHERE period_id=?");
    query.addBindValue(periodId);
    return query.exec();
}

bool DatabaseHandler::periodHasProjects(int periodId) const {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM tupitube_project WHERE period_id = ?");
    query.addBindValue(periodId);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    return false;
}

// === Render support ===

DatabaseHandler::RenderProjectInfo DatabaseHandler::getProjectRenderInfo(int projectId) const
{
    RenderProjectInfo info;
    info.found = false;
    info.studentId = -1;

    QSqlQuery query;
    query.prepare("SELECT student_id, filename, title FROM tupitube_project WHERE project_id = ?");
    query.addBindValue(projectId);
    if (!query.exec() || !query.next()) {
        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::getProjectRenderInfo()] - Project not found:" << projectId;
        #endif
        return info;
    }

    info.found = true;
    info.studentId = query.value(0).toInt();
    info.filename = query.value(1).toString();
    info.title = query.value(2).toString();
    return info;
}

bool DatabaseHandler::updateProjectLastRendered(int projectId)
{
    QSqlQuery query;
    query.prepare("UPDATE tupitube_project SET last_rendered_at = datetime('now') WHERE project_id = ?");
    query.addBindValue(projectId);
    bool ok = query.exec();
    #ifdef TUP_DEBUG
        if (!ok)
            qWarning() << "[DatabaseHandler::updateProjectLastRendered()] - Error:" << query.lastError().text();
    #endif
    return ok;
}

bool DatabaseHandler::touchProjectUpdatedAt(const QString &filename)
{
    QSqlQuery query;
    query.prepare("UPDATE tupitube_project SET updated_at = datetime('now') WHERE filename = ?");
    query.addBindValue(filename);
    bool ok = query.exec();
    #ifdef TUP_DEBUG
        if (!ok)
            qWarning() << "[DatabaseHandler::touchProjectUpdatedAt()] - Error:" << query.lastError().text();
    #endif
    return ok;
}

bool DatabaseHandler::renameProjectTitle(int projectId, const QString &newTitle)
{
    QSqlQuery query;
    query.prepare("UPDATE tupitube_project SET title = ?, updated_at = datetime('now') WHERE project_id = ?");
    query.addBindValue(newTitle);
    query.addBindValue(projectId);
    bool ok = query.exec();
    #ifdef TUP_DEBUG
        if (!ok)
            qWarning() << "[DatabaseHandler::renameProjectTitle()] - Error:" << query.lastError().text();
    #endif
    return ok;
}

// === Grade management ===

bool DatabaseHandler::saveGrade(int projectId, int studentId, int teacherStudentId,
                                 int periodId, int classId, const QString &grade,
                                 const QString &comments)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::saveGrade()] - projectId:" << projectId
                 << "studentId:" << studentId << "grade:" << grade;
    #endif

    QSqlQuery query(db);

    // Check whether a grade record already exists for this key
    query.prepare("SELECT grade_id FROM tupitube_grade "
                  "WHERE project_id = ? AND student_id = ? AND teacher_student_id = ? "
                  "AND period_id = ? AND class_id = ?");
    query.addBindValue(projectId);
    query.addBindValue(studentId);
    query.addBindValue(teacherStudentId);
    query.addBindValue(periodId);
    query.addBindValue(classId);

    if (!query.exec()) {
        #ifdef TUP_DEBUG
            qWarning() << "[DatabaseHandler::saveGrade()] - Error (select):" << query.lastError().text();
        #endif
        return false;
    }

    bool ok;
    if (query.next()) {
        // Record exists — update grade and comments
        QSqlQuery upd(db);
        upd.prepare("UPDATE tupitube_grade "
                    "SET grade = ?, comments = ?, updated_at = datetime('now') "
                    "WHERE project_id = ? AND student_id = ? AND teacher_student_id = ? "
                    "AND period_id = ? AND class_id = ?");
        upd.addBindValue(grade);
        upd.addBindValue(comments);
        upd.addBindValue(projectId);
        upd.addBindValue(studentId);
        upd.addBindValue(teacherStudentId);
        upd.addBindValue(periodId);
        upd.addBindValue(classId);
        ok = upd.exec();
        #ifdef TUP_DEBUG
            if (!ok)
                qWarning() << "[DatabaseHandler::saveGrade()] - Error (update):" << upd.lastError().text();
        #endif
    } else {
        // No record yet — insert
        QSqlQuery ins(db);
        ins.prepare("INSERT INTO tupitube_grade "
                    "(project_id, student_id, teacher_student_id, period_id, class_id, "
                    " grade, comments, created_at, updated_at) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))");
        ins.addBindValue(projectId);
        ins.addBindValue(studentId);
        ins.addBindValue(teacherStudentId);
        ins.addBindValue(periodId);
        ins.addBindValue(classId);
        ins.addBindValue(grade);
        ins.addBindValue(comments);
        ok = ins.exec();
        #ifdef TUP_DEBUG
            if (!ok)
                qWarning() << "[DatabaseHandler::saveGrade()] - Error (insert):" << ins.lastError().text();
        #endif
    }

    return ok;
}

DatabaseHandler::GradeInfo DatabaseHandler::getGrade(int projectId, int studentId) const
{
    GradeInfo info;
    info.found = false;
    info.gradeId = -1;
    info.grade = QString();

    QSqlQuery query(db);
    query.prepare("SELECT grade_id, grade, comments, updated_at FROM tupitube_grade "
                  "WHERE project_id = ? AND student_id = ? "
                  "ORDER BY updated_at DESC LIMIT 1");
    query.addBindValue(projectId);
    query.addBindValue(studentId);
    if (!query.exec() || !query.next())
        return info;

    info.found = true;
    info.gradeId = query.value(0).toInt();
    info.grade = query.value(1).toString();
    info.comments = query.value(2).toString();
    info.updatedAt = query.value(3).toString();
    return info;
}

int DatabaseHandler::getProjectIdFromFilename(const QString &filename)
{
    QSqlQuery query;
    query.prepare("SELECT project_id FROM tupitube_project WHERE filename = :filename");
    query.bindValue(":filename", filename);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return -1; // Return -1 if not found
}
