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
#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include "netproject.h"

#include <QString>
#include <QtSql>

class DatabaseHandler
{
    public:

        struct ProjectInfo
        {
            QString title;
            QString owner;
            QString description;
            QString date;
            QString file;
        };

        struct UserInfo
        {
            int userId;
            QString username;
            QString name;
            QString password;
            bool isEnabled;
            bool isCreator;
        };

        enum MediaType { DeskImg = 0, DeskAnim, DeskStory };

        DatabaseHandler();
        ~DatabaseHandler();
        
        bool addProject(const NetProject *project);

	QString getUserID(const QString &username) const;
        bool addWork(const QString &projectID, const QString &type, const QString &owner, const QString &title, 
                     const QString &topics, const QString &desc, const QString &filename, bool portrait);

        bool addStoryFrame(const QString &storyboard, const QString &id, const QString &owner, const QString &title, const QString &topics, 
                           const QString &description, const QString &filename,  const QString &duration);
        bool addStoryboard(const QString &id, const QString &owner, const QString &title, const QString &topics, 
                           const QString &description, const QString &path);

        bool slugExists(const QString &slug, const QString &owner);
        QString storyboardID(const QString &uid, const QString &directory) const;
        QList<DatabaseHandler::ProjectInfo> userProjects(int userID, const QString &login);
        QList< DatabaseHandler::ProjectInfo> partnerProjects(int userID);

        QString exists(const QString &projectName, const QString &ownerID) const;
        bool accessIsConfirmed(const QString &filename, int userID);
        QString userID(const QString &login) const;
        bool addLog(const QString &type, const QString &filename, const QString &ip);

        // User management methods (for classroom administration)
        QList<UserInfo> getAllUsers() const;
        bool addUser(const QString &username, const QString &name, const QString &password, bool isEnabled, bool isCreator);
        bool updateUser(int userId, const QString &username, const QString &name, const QString &password, bool isEnabled, bool isCreator);
        bool removeUser(int userId);
        bool usernameExists(const QString &username) const;

        // Collaboration management methods (teacher-only from server GUI)
        struct CollaboratorInfo
        {
            int collaborationId;
            int userId;
            QString username;
            QString name;
            int permissionLevel;
        };

        struct ProjectRecord
        {
            int projectId;
            QString title;
            QString filename;
            int ownerId;
            QString ownerUsername;
            QString description;
            QString createdAt;
            bool isShared;
        };

        QList<ProjectRecord> getAllProjects() const;
        QList<CollaboratorInfo> getProjectCollaborators(int projectId) const;
        bool addCollaborator(int projectId, int userId, int permissionLevel = 1);
        bool removeCollaborator(int projectId, int userId);
        bool deleteProject(int projectId);
        QString getProjectFilename(int projectId) const;
        int getProjectOwnerId(int projectId) const;
        QString getOwnerUsername(int projectId) const;
        bool createEmptyProject(const QString &title, const QString &description, int ownerId, 
                                const QString &filename, const QList<int> &collaboratorIds);

        // Chat message storage (for teacher review)
        struct ChatMessage
        {
            int chatId;
            int projectId;
            int userId;
            QString username;
            QString message;
            QString messageType;  // "chat", "notice", "wall"
            QString createdAt;
        };

        bool saveChatMessage(int projectId, int userId, const QString &username, 
                             const QString &message, const QString &messageType = "chat");
        QList<ChatMessage> getChatHistory(int projectId = -1, int limit = 500) const;
        QList<ChatMessage> getChatHistoryByDate(const QString &fromDate, const QString &toDate) const;
        bool clearChatHistory(int projectId = -1);
        
    private:
        QString incomingFolderID(const QString &uid, const QString &type) const;
        QString worksPublicPolicy(const QString &owner) const;
        QString projectKey(const QString &filename) const;
        QString userLogin(const QString &owner) const;
        int count(const QString &sql);
};

#endif
