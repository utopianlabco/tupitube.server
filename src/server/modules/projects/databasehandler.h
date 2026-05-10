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
    // Class and Period management
    struct ClassInfo {
        int classId;
        QString name;
        int year;
        QString description;
    };
    struct PeriodInfo {
        int periodId;
        QString name;
        int year;
        QString startDate;
        QString endDate;
    };

    QList<ClassInfo> getAllClasses() const;
    // Create or update the database schema (tables, constraints)
    void createDatabaseSchema();
    bool addClass(const QString &name, int year, const QString &description);
    bool updateClass(int classId, const QString &name, int year, const QString &description);
    bool removeClass(int classId);

    QList<PeriodInfo> getAllPeriods() const;
    bool addPeriod(const QString &name, int year, const QString &startDate, const QString &endDate);
    bool updatePeriod(int periodId, const QString &name, int year, const QString &startDate, const QString &endDate);
    bool removePeriod(int periodId);
    public:

        struct ProjectInfo
        {
            QString title;
            QString owner;
            QString description;
            QString date;
            QString file;
            int classId;
            QString className;
            int periodId;
            QString periodName;
            bool groupProject;
        };

        struct StudentInfo
        {
            int studentId;
            QString studentname;
            QString name;
            QString password;
            bool isEnabled;
            bool isCreator;
            int classId;
            QString className;
        };

        enum MediaType { DeskImg = 0, DeskAnim, DeskStory };

        DatabaseHandler();
        ~DatabaseHandler();
        
        bool addProject(const NetProject *project);

	QString getStudentID(const QString &studentname) const;
        bool addWork(const QString &projectID, const QString &type, const QString &owner, const QString &title, 
                     const QString &topics, const QString &desc, const QString &filename, bool portrait);

        bool addStoryFrame(const QString &storyboard, const QString &id, const QString &owner, const QString &title, const QString &topics, 
                           const QString &description, const QString &filename,  const QString &duration);
        bool addStoryboard(const QString &id, const QString &owner, const QString &title, const QString &topics, 
                           const QString &description, const QString &path);

        bool slugExists(const QString &slug, const QString &owner);
        QString storyboardID(const QString &uid, const QString &directory) const;
        QList<DatabaseHandler::ProjectInfo> studentProjects(int studentID, const QString &login);
        QList< DatabaseHandler::ProjectInfo> partnerProjects(int studentID);

        QString exists(const QString &projectName, const QString &ownerID) const;
        bool accessIsConfirmed(const QString &filename, int studentID);
        QString studentID(const QString &login) const;
        bool addLog(const QString &type, const QString &filename, const QString &ip);

        // Student management methods (for classroom administration)
        QList<StudentInfo> getAllStudents() const;
        bool addStudent(const QString &studentname, const QString &name, const QString &password, bool isEnabled, bool isCreator, const QString &studentClass);
        bool updateStudent(int studentId, const QString &studentname, const QString &name, const QString &password, bool isEnabled, bool isCreator, const QString &studentClass);
        bool removeStudent(int studentId);
        bool studentnameExists(const QString &studentname) const;

        // Collaboration management methods (teacher-only from server GUI)
        struct CollaboratorInfo
        {
            int collaborationId;
            int studentId;
            QString studentname;
            QString name;
            int permissionLevel;
        };

        struct ProjectRecord
        {
            int projectId;
            QString title;
            QString filename;
            int ownerId;
            QString ownerStudentname;
            QString description;
            QString createdAt;
            bool isShared;
            int classId;
            QString className;
            int periodId;
            QString periodName;
            bool groupProject;
            QString lastRenderedAt; // empty if never rendered
        };

        QList<ProjectRecord> getAllProjects() const;
        QList<CollaboratorInfo> getProjectCollaborators(int projectId) const;
        bool addCollaborator(int projectId, int studentId, int permissionLevel = 1);
        bool removeCollaborator(int projectId, int studentId);
        bool deleteProject(int projectId);
        QString getProjectFilename(int projectId) const;
        int getProjectOwnerId(int projectId) const;
        QString getOwnerStudentname(int projectId) const;
        bool createEmptyProject(const QString &title, const QString &description, int ownerId, 
                    const QString &filename, const QList<int> &collaboratorIds, int periodId);

        // Chat message storage (for teacher review)
        struct ChatMessage
        {
            int chatId;
            int projectId;
            int studentId;
            QString studentname;
            QString message;
            QString messageType;  // "chat", "notice", "wall"
            QString createdAt;
        };

        bool saveChatMessage(int projectId, int studentId, const QString &studentname, 
                             const QString &message, const QString &messageType = "chat");
        QList<ChatMessage> getChatHistory(int projectId = -1, int limit = 500) const;
        QList<ChatMessage> getChatHistoryByDate(const QString &fromDate, const QString &toDate) const;
        bool clearChatHistory(int projectId = -1);

        // Render support (used by ProjectRenderer)
        struct RenderProjectInfo
        {
            bool found;
            int studentId;
            QString filename;
            QString title;
        };

        RenderProjectInfo getProjectRenderInfo(int projectId) const;
        bool updateProjectLastRendered(int projectId);

        // Grade management (teacher assigns grade per project)
        struct GradeInfo
        {
            bool found;
            int gradeId;
            double grade;
            QString comments;
            QString updatedAt;
        };

        bool saveGrade(int projectId, int studentId, int teacherStudentId,
                       int periodId, int classId, double grade,
                       const QString &comments);
        GradeInfo getGrade(int projectId, int studentId) const;

    private:
        QString incomingFolderID(const QString &uid, const QString &type) const;
        QString worksPublicPolicy(const QString &owner) const;
        QString projectKey(const QString &filename) const;
        QString studentLogin(const QString &owner) const;
        int count(const QString &sql);
};

#endif
