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
#include <QSqlDatabase>

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
    void initDataBase();
    void createDatabaseSchema();
    bool addClass(const QString &name, int year, const QString &description);
    bool updateClass(int classId, const QString &name, int year, const QString &description);
    bool removeClass(int classId);
    bool classHasStudents(int classId) const;
    bool classHasProjects(int classId) const;
    int getStudentCountForClass(int classId) const;

    QList<PeriodInfo> getAllPeriods() const;
    bool addPeriod(const QString &name, int year, const QString &startDate, const QString &endDate);
    bool updatePeriod(int periodId, const QString &name, int year, const QString &startDate, const QString &endDate);
    bool removePeriod(int periodId);
    bool periodHasProjects(int periodId) const;

    int getProjectIdFromFilename(const QString &filename);

    // Server-authoritative collaboration persistence
    struct ProjectRevisionInfo
    {
        bool found = false;
        qint64 currentRevision = 0;
        qint64 snapshotRevision = 0;
        QString snapshotChecksum;
        QString snapshotUpdatedAt;
    };

    struct ProjectEventRecord
    {
        QString eventUuid;
        int projectId = -1;
        QString commandId;
        qint64 revision = -1;
        int eventIndex = -1;
        QString eventType;
        QString payload;
        QString createdAt;
    };

    struct ProjectCommandRecord
    {
        bool found = false;
        int projectId = -1;
        QString commandId;
        int studentId = -1;
        QString clientId;
        QString commandType;
        qint64 baseRevision = 0;
        QString dependsOnCommandId;
        QString requestHash;
        QString status;
        QString errorCode;
        QString message;
        qint64 committedRevision = -1;
        QString createdAt;
        QString updatedAt;
        QString completedAt;
    };

    ProjectRevisionInfo getProjectRevisionInfo(int projectId) const;
    ProjectCommandRecord getProjectCommand(int projectId, const QString &commandId) const;
    QList<ProjectEventRecord> getProjectEventsAfter(int projectId,
                                                    qint64 revision,
                                                    int eventIndex,
                                                    int limit = 501) const;

    bool insertProjectCommand(int projectId, const QString &commandId,
                              int studentId = -1,
                              const QString &clientId = QString(),
                              const QString &commandType = QString(),
                              qint64 baseRevision = 0,
                              const QString &dependsOnCommandId = QString(),
                              const QString &requestHash = QString(),
                              const QString &status = QStringLiteral("processing"));

    bool updateProjectCommandResult(int projectId, const QString &commandId,
                                    const QString &status,
                                    const QString &errorCode = QString(),
                                    const QString &message = QString());

    // Call only after the corresponding .tup snapshot has been saved successfully.
    // The command result, project revision, and authoritative event are committed atomically in SQLite.
    bool finalizeCommittedProjectCommand(int projectId, const QString &commandId,
                                         qint64 *committedRevision,
                                         const QString &eventUuid,
                                         const QString &eventType,
                                         const QString &eventPayload,
                                         const QString &snapshotChecksum = QString());

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
        bool addStudent(const QString &studentname, const QString &name, const QString &password, bool isEnabled, bool isCreator, int classId);
        bool updateStudent(int studentId, const QString &studentname, const QString &name, const QString &password, bool isEnabled, bool isCreator, int classId);
        bool removeStudent(int studentId);
        bool studentnameExists(const QString &studentname) const;
        int getClassIdByName(const QString &name) const;

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
            QString updatedAt;
        };

        QList<ProjectRecord> getAllProjects() const;
        ProjectRecord getProjectById(int projectId) const;
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
        bool touchProjectUpdatedAt(const QString &filename);

        // Grade management (teacher assigns grade per project)
        struct GradeInfo
        {
            bool found;
            int gradeId;
            QString grade;
            QString comments;
            QString updatedAt;
        };

        bool renameProjectTitle(int projectId, const QString &newTitle);
        bool saveGrade(int projectId, int studentId, int teacherStudentId,
                       int periodId, int classId, const QString &grade,
                       const QString &comments);
        GradeInfo getGrade(int projectId, int studentId) const;

    private:
        QSqlDatabase db;
        QString incomingFolderID(const QString &uid, const QString &type) const;
        QString worksPublicPolicy(const QString &owner) const;
        QString projectKey(const QString &filename) const;
        QString studentLogin(const QString &owner) const;
        int count(const QString &sql);
};

#endif
