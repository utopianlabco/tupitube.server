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
#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include "projectlist.h"
#include "netproject.h"
#include "databasehandler.h"
#include "observer.h"
#include "newprojectparser.h"
#include "tupexportinterface.h"

#include <QString>
#include <QDomDocument>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHash>
#include <QList>

class ProjectManager : public Observer
{
    Q_OBJECT

    public:
        enum VideoCase { Normal = 0, Fixed, FpsOne };
        // enum AndroidVersion { Newer = 0, Older };
        ProjectManager();
        ~ProjectManager();
        
        void handlePackage(PackageBase *const pkg);

    // private slots:
        // void updateProcessStatus(int exitCode, QProcess::ExitStatus exitStatus);
        // void postFinished(QNetworkReply*);
        // void slotError(QNetworkReply::NetworkError error);

    private:
        void createProject(Connection *connection);
        void openProject(const QString &filename, const QString &owner, Connection *connection);
        void importProject(Connection *connection, const QString &path, const QByteArray &data);
        void createImage(Connection *connection, int frame, int scene, const QString &title, const QString &topics,
                         const QString &description);
        void createVideo(Connection *connection, const QString &title, const QString &topics, const QString &description,
                         int fps, const QList<int> scenes);
        void createStoryboard(Connection *connection, int sceneIndex);
        void updateStoryboard(Connection *connection, int sceneIndex, const QString &storyXml);

        bool handleProjectRequest(const QString &projectID, const QString strRequest);

        bool saveProject(const QString &projectID, bool quiet);
        void closeProject(const QString &name);

        void closeConnection(Connection *connection);
        void sendToProjectMembers(Connection *connection, QDomDocument &doc);

        void registerProject(Connection *connection, const QString &uid, const QString &filename, NetProject *project);
        void listUserProjects(Connection *connection);
        QString currentDate() const;
        void loadVideoPlugin();

        bool resizeVideo(const QString &code, const QString &input, const QSize &size);

        // void postWork();

        QHash<QString, NetProject *> m_openedProjects;
        DatabaseHandler *m_dbHandler;
        QHash<QString, QList<Connection *> > m_connectionList;
        NewProjectParser m_connectionData;
        TupExportInterface *m_exporter;
        Connection *m_connection;
        QString m_videoUrl;
        int m_formatVersion;

        QNetworkAccessManager *networkManager;
        // static QString BROWSER_FINGERPRINT;
};

#endif
