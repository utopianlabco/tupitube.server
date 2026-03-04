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

#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QDebug>

DatabaseHandler::DatabaseHandler()
{
}

DatabaseHandler::~DatabaseHandler()
{
}

QString DatabaseHandler::incomingFolderID(const QString &uid, const QString &type) const
{
    QString folderID = "";
    QString typeKey = type.toLower();
    QString sql = "SELECT collection_id FROM tupitube_collection WHERE owner_id=" + uid
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
    QString sql = "SELECT works_public_policy FROM tupitube_user WHERE user_id=" + owner;
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

    QString sql = "SELECT files_public_policy, projects_public_policy FROM tupitube_user WHERE user_id=" + owner;
    QSqlQuery query(sql);
    if (query.next() && query.first()) {
        isPublic = query.value(0).toString();
        isShared = query.value(1).toString();
    }
    query.clear();

    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addProject()] - SQL: " << sql;
    #endif

    QString name = project->getName().replace("'", "\\'");
    QString description = project->getDescription().replace("'", "\\'");

    sql = "INSERT INTO tupitube_project (title, description, owner_id, filename, is_public, is_shared, created_at, updated_at) VALUES(";
    sql += "'" + name + "', ";
    sql += "'" + description + "', ";
    sql += owner + ", ";
    sql += "'" + project->filename() + "', ";
    sql += isPublic + ", ";
    sql += isShared + ", ";    
    sql += "datetime('now'), ";
    sql += "datetime('now'))";

    #ifdef TUP_DEBUG
        qDebug() << "DatabaseHandler::addProject() SQL: " << sql;
    #endif

    bool isOk = query.exec(sql); 
 
    return isOk;
}

QString DatabaseHandler::getUserID(const QString &username) const
{
    QString uid = "1";

    if (username.compare("anonymous") != 0) {
        QSqlQuery query;
        query.exec("SELECT id FROM user WHERE username='" + username + "'");
        if (query.next()) {
            uid = query.value(0).toString();
            qDebug() << "[DatabaseHandler::getUserID()] - UID: " <<  uid;
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[DatabaseHandler::getUserID()] - Fatal Error: Invalid username -> " << username;
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

    QString sql = "INSERT INTO tupi_work (project_id, collection_id, type, title, topics, description, owner_id, filename, is_public, portrait, created_at, updated_at) VALUES(";
    sql += "'" + projectKey(projectID) + "', ";
    sql += "'" + collectionID + "', ";
    sql += "'" + type + "', ";
    sql += "'" + workTitle.replace("'", "''") + "', ";
    sql += "'" + workTopics.replace("'", "") + "', ";
    sql += "'" + workDescription.replace("'", "''") + "', ";
    sql += owner + ", ";
    sql += "'" + filename + "', ";
    sql += isPublic + ", ";
    sql += isPortrait + ", ";
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

    QString sql = "INSERT INTO tupitube_work (project_id, collection_id, type, title, topics, description, owner_id, filename, is_public, visits, created_at, updated_at, duration) VALUES(";
    sql += projectID + ", ";
    sql += storyboard + ", ";
    sql += "'frame', ";
    sql += "'" + workTitle.replace("'", "\\'") + "', ";
    sql += "'" + workTopics.replace("'", "") + "', ";
    sql += "'" + workDescription.replace("'", "\\'") + "', ";
    sql += owner + ", ";
    sql += "'" + filename + "', ";
    sql += isPublic + ", ";
    sql += "0, ";
    sql += "datetime('now'), ";
    sql += "datetime('now'), ";
    sql += "'" + duration + "')";

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
               qDebug() << "[DatabaseHandler::addStoryboard()] - Fatal Error: Storyboard incoming folder doesn't exist for user " << owner;
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

    QString sql = "INSERT INTO tupitube_collection (parent_id, type, title, topics, description, owner_id, is_public, visits, likes, project_id, path, created_at, updated_at, slug) VALUES(";
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
    QString sql = "SELECT count(*) FROM tupitube_collection WHERE slug='" + slug + "' AND owner_id=" + owner;

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
    QString sql = "SELECT collection_id FROM tupitube_collection WHERE owner_id=" + uid + " AND path ='" + directory + "'";

    QSqlQuery query = QSqlQuery(sql);
    if (query.next() && query.first())
        id = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::storyboardID()] - SQL: " << sql;
    #endif

    return id;
}

QList< DatabaseHandler::ProjectInfo> DatabaseHandler::userProjects(int userID, const QString &login)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::userProjects()]";
    #endif

    QList<DatabaseHandler::ProjectInfo> list;

    QString sql = "SELECT title, description, filename, created_at FROM tupitube_project WHERE owner_id=" + QString::number(userID) + " ORDER BY created_at DESC";
    QSqlQuery query(sql);
    QString name = "";
    QString description = "";
    QString date = "";

    while (query.next()) {
           DatabaseHandler::ProjectInfo record;
           record.title = query.value(0).toString();
           record.owner = login; 
           record.description = query.value(1).toString();
           record.file = query.value(2).toString();
           QDateTime date = query.value(3).toDateTime();
           record.date = date.toString("dd/MM/yyyy hh:mm"); 

           list.append(record);
    }
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::userProjects()] - SQL: " << sql;
    #endif

    return list;
}

QList< DatabaseHandler::ProjectInfo> DatabaseHandler::partnerProjects(int userID)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::partnerProjects()]";
    #endif

    QList<DatabaseHandler::ProjectInfo> list;
    QList<QString> projects;

    QString sql = "SELECT project_id FROM tupitube_collaboration WHERE user_id=" + QString::number(userID);
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
         QSqlQuery query("SELECT title, description, owner_id, filename, created_at FROM tupitube_project WHERE project_id=" + projects.at(i));
         QString name = "";
         QString description = "";
         QString date = "";

         while (query.next()) {
                DatabaseHandler::ProjectInfo record;
                record.title = query.value(0).toString();
                record.description = query.value(1).toString();
                QString owner = query.value(2).toString();
                record.owner = userLogin(owner);
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

bool DatabaseHandler::accessIsConfirmed(const QString &projectID, int userID)
{
    QString uid = QString::number(userID);

    QString sql = "SELECT count(*) FROM tupitube_project WHERE project_id=" + projectID + " AND owner_id=" + uid;
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

    sql = "SELECT count(*) FROM tupitube_collaboration WHERE user_id=" + uid + " AND project_id=" + projectID;
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

QString DatabaseHandler::userLogin(const QString &owner) const
{
    QString login = "unknown";
 
    QString sql = "SELECT username FROM tupitube_user WHERE user_id=" + owner;
    QSqlQuery query(sql);
    if (query.first())
        login = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::userLogin()] - SQL: " << sql;
    #endif

    return login;
}

QString DatabaseHandler::userID(const QString &login) const
{
    QString id = "unknown";

    QString sql = "SELECT user_id FROM tupitube_user WHERE username='" + login + "'";
    QSqlQuery query(sql);
    if (query.first())
        id = query.value(0).toString();
    query.clear();

    #ifdef TUP_DEBUG
           qWarning() << "[DatabaseHandler::userID()] - SQL: " << sql;
    #endif

    return id;
}

QString DatabaseHandler::exists(const QString &filename, const QString &ownerID) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::exists()]";
    #endif

    QString sql = "SELECT project_id FROM tupitube_project WHERE filename='" + filename + "' AND owner_id=" + ownerID;
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

QList<DatabaseHandler::UserInfo> DatabaseHandler::getAllUsers() const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::getAllUsers()]";
    #endif

    QList<UserInfo> users;
    QString sql = "SELECT user_id, username, name, password, is_enabled, is_creator FROM tupitube_user ORDER BY username";
    QSqlQuery query(sql);

    while (query.next()) {
        UserInfo user;
        user.userId = query.value(0).toInt();
        user.username = query.value(1).toString();
        user.name = query.value(2).toString();
        user.password = query.value(3).toString();
        user.isEnabled = query.value(4).toBool();
        user.isCreator = query.value(5).toBool();
        users.append(user);
    }

    return users;
}

bool DatabaseHandler::addUser(const QString &username, const QString &name, const QString &password, bool isEnabled, bool isCreator)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addUser()] - username:" << username;
    #endif

    if (usernameExists(username)) {
        #ifdef TUP_DEBUG
            qDebug() << "[DatabaseHandler::addUser()] - Username already exists:" << username;
        #endif
        return false;
    }

    QString sql = "INSERT INTO tupitube_user (username, name, password, is_enabled, is_creator) VALUES (";
    sql += "'" + username + "', ";
    sql += "'" + name + "', ";
    sql += "'" + password + "', ";
    sql += QString::number(isEnabled ? 1 : 0) + ", ";
    sql += QString::number(isCreator ? 1 : 0) + ")";

    QSqlQuery query;
    bool isOk = query.exec(sql);

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::addUser()] - SQL:" << sql;
        if (!isOk)
            qWarning() << "[DatabaseHandler::addUser()] - Error:" << query.lastError().text();
    #endif

    return isOk;
}

bool DatabaseHandler::updateUser(int userId, const QString &username, const QString &name, const QString &password, bool isEnabled, bool isCreator)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::updateUser()] - userId:" << userId;
    #endif

    QString sql = "UPDATE tupitube_user SET ";
    sql += "username = '" + username + "', ";
    sql += "name = '" + name + "', ";
    if (!password.isEmpty()) {
        sql += "password = '" + password + "', ";
    }
    sql += "is_enabled = " + QString::number(isEnabled ? 1 : 0) + ", ";
    sql += "is_creator = " + QString::number(isCreator ? 1 : 0) + ", ";
    sql += "updated_at = datetime('now') ";
    sql += "WHERE user_id = " + QString::number(userId);

    QSqlQuery query;
    bool isOk = query.exec(sql);

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::updateUser()] - SQL:" << sql;
        if (!isOk)
            qWarning() << "[DatabaseHandler::updateUser()] - Error:" << query.lastError().text();
    #endif

    return isOk;
}

bool DatabaseHandler::removeUser(int userId)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::removeUser()] - userId:" << userId;
    #endif

    QString sql = "DELETE FROM tupitube_user WHERE user_id = " + QString::number(userId);

    QSqlQuery query;
    bool isOk = query.exec(sql);

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::removeUser()] - SQL:" << sql;
        if (!isOk)
            qWarning() << "[DatabaseHandler::removeUser()] - Error:" << query.lastError().text();
    #endif

    return isOk;
}

bool DatabaseHandler::usernameExists(const QString &username) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::usernameExists()] - username:" << username;
    #endif

    QString sql = "SELECT COUNT(*) FROM tupitube_user WHERE username = '" + username + "'";
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
    QString sql = "SELECT p.project_id, p.title, p.filename, p.owner_id, u.username, p.description, "
                  "p.created_at, p.is_shared FROM tupitube_project p "
                  "LEFT JOIN tupitube_user u ON p.owner_id = u.user_id "
                  "ORDER BY p.created_at DESC";
    QSqlQuery query(sql);

    while (query.next()) {
        ProjectRecord record;
        record.projectId = query.value(0).toInt();
        record.title = query.value(1).toString();
        record.filename = query.value(2).toString();
        record.ownerId = query.value(3).toInt();
        record.ownerUsername = query.value(4).toString();
        record.description = query.value(5).toString();
        record.createdAt = query.value(6).toString();
        record.isShared = query.value(7).toBool();
        projects.append(record);
    }

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
    QString sql = "SELECT c.collaboration_id, c.user_id, u.username, u.name, c.permission_level "
                  "FROM tupitube_collaboration c "
                  "LEFT JOIN tupitube_user u ON c.user_id = u.user_id "
                  "WHERE c.project_id = " + QString::number(projectId) + " "
                  "ORDER BY u.username";
    QSqlQuery query(sql);

    while (query.next()) {
        CollaboratorInfo info;
        info.collaborationId = query.value(0).toInt();
        info.userId = query.value(1).toInt();
        info.username = query.value(2).toString();
        info.name = query.value(3).toString();
        info.permissionLevel = query.value(4).toInt();
        collaborators.append(info);
    }

    #ifdef TUP_DEBUG
        qWarning() << "[DatabaseHandler::getProjectCollaborators()] - Found" << collaborators.size() << "collaborators";
    #endif

    return collaborators;
}

bool DatabaseHandler::addCollaborator(int projectId, int userId, int permissionLevel)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::addCollaborator()] - projectId:" << projectId << "userId:" << userId;
    #endif

    // Check if collaboration already exists
    QString checkSql = "SELECT COUNT(*) FROM tupitube_collaboration WHERE project_id = " 
                       + QString::number(projectId) + " AND user_id = " + QString::number(userId);
    QSqlQuery checkQuery(checkSql);
    if (checkQuery.first() && checkQuery.value(0).toInt() > 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[DatabaseHandler::addCollaborator()] - Collaboration already exists";
        #endif
        return false;
    }

    QString sql = "INSERT INTO tupitube_collaboration (project_id, user_id, permission_level) VALUES (";
    sql += QString::number(projectId) + ", ";
    sql += QString::number(userId) + ", ";
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

bool DatabaseHandler::removeCollaborator(int projectId, int userId)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::removeCollaborator()] - projectId:" << projectId << "userId:" << userId;
    #endif

    QString sql = "DELETE FROM tupitube_collaboration WHERE project_id = " + QString::number(projectId) 
                  + " AND user_id = " + QString::number(userId);

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
                                          const QString &filename, const QList<int> &collaboratorIds)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::createEmptyProject()] - title:" << title << "owner:" << ownerId;
    #endif

    bool isShared = !collaboratorIds.isEmpty();

    QString sql = "INSERT INTO tupitube_project (title, description, owner_id, filename, is_shared) VALUES (";
    sql += "'" + title + "', ";
    sql += "'" + description + "', ";
    sql += QString::number(ownerId) + ", ";
    sql += "'" + filename + "', ";
    sql += QString::number(isShared ? 1 : 0) + ")";

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
    for (int userId : collaboratorIds) {
        if (userId != ownerId) {  // Don't add owner as collaborator
            addCollaborator(projectId, userId);
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
    query.exec("SELECT owner_id FROM tupitube_project WHERE project_id = " + QString::number(projectId));
    if (query.next())
        return query.value(0).toInt();
    return -1;
}

QString DatabaseHandler::getOwnerUsername(int projectId) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::getOwnerUsername()] - projectId:" << projectId;
    #endif

    QString sql = "SELECT u.username FROM tupitube_project p "
                  "LEFT JOIN tupitube_user u ON p.owner_id = u.user_id "
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

bool DatabaseHandler::saveChatMessage(int projectId, int userId, const QString &username, 
                                      const QString &message, const QString &messageType)
{
    #ifdef TUP_DEBUG
        qDebug() << "[DatabaseHandler::saveChatMessage()] - User:" << username << "Type:" << messageType;
    #endif

    QSqlQuery query;
    query.prepare("INSERT INTO tupitube_chat (project_id, user_id, username, message, message_type) "
                  "VALUES (:projectId, :userId, :username, :message, :messageType)");
    
    if (projectId > 0)
        query.bindValue(":projectId", projectId);
    else
        query.bindValue(":projectId", QVariant(QVariant::Int));  // NULL for global chat
    
    query.bindValue(":userId", userId);
    query.bindValue(":username", username);
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

    QString sql = "SELECT chat_id, project_id, user_id, username, message, message_type, created_at "
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
        msg.userId = query.value(2).toInt();
        msg.username = query.value(3).toString();
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
    query.prepare("SELECT chat_id, project_id, user_id, username, message, message_type, created_at "
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
        msg.userId = query.value(2).toInt();
        msg.username = query.value(3).toString();
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