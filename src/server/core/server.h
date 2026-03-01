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
#ifndef _SERVER_H__
#define _SERVER_H__

#include "user.h"
#include "observer.h"
#include "usermanager.h"

#include <QStringList>
#include <QTcpServer>
#include <QSqlDatabase>
#include <QDomDocument>

class Connection;
class ProjectManager;
class CommunicationManager;

class TcpServer : public QTcpServer
{
    Q_OBJECT
    
    public:
        TcpServer(QObject *parent = nullptr);
        ~TcpServer();

        void sendToAll(const QDomDocument &pkg);
        bool openConnection(const QString &host, int port);
        
        void sendToAdmins(const QString &str);
        
        void addAdmin(Connection *connection);
        UserManager *userManager() const;
        
        void addObserver(Observer *observer);
        bool removeObserver(Observer *observer);

        int connectionCount() const;

    signals:
        void connectionCountChanged(int count);
        void userConnected(const QString &username, const QString &ip);
        void userDisconnected(const QString &username);
        void logMessage(const QString &message, const QString &level);
        
    private slots:
        void sendToAll(const QString &msg);
        void handlePackage(Connection *client, const QString &root, const QString &packages);
        void removeConnection(Connection *connection);
    
    private:
        void initDataBase();
        void createDatabaseSchema();
        void handle(Connection *connection);

        // bool verifyPassword(const QString &login, const QString &password);
        // bool userExists(const QString &login);
        // User *userData(const QString &login);
        
    protected:
        void incomingConnection(qintptr socketDescriptor);
        
    private:
        QList<Connection *> m_connections;
        QList<Connection *> m_managers;
        UserManager *m_userManager;
        ProjectManager *m_projectManager;
        CommunicationManager *m_communicationManager;
        QList<Observer *> m_observers;
        QSqlDatabase m_db;
};

#endif
