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
#ifndef CONNECTION_H
#define CONNECTION_H

#include "serverclient.h"
#include "notification.h"
#include "user.h"

#include <QThread>
#include <QTimerEvent>
#include <QQueue>
#include <QHash>
#include <QVariant>

class TupProjectRequest;
class TcpServer;

class Connection : public QThread
{
    Q_OBJECT

    public:
        Connection(qintptr socketDescriptor, TcpServer *server);
        ~Connection();
        
        void run();
        
        void appendTextReaded(const QString &package);
        
        void sendStringToClient(const QString &text);
        void sendStringToClient(QDomDocument &doc, bool sign = true);
        void sendToAll(const QString &text);
        void sendToAll(QDomDocument &doc, bool sign = true);

        void sendFileToClient(const QString &path);
        
        void setData(int key, const QVariant &value);
        QVariant data(int key) const;
        
        Client *client() const;
        TcpServer *server() const;
        
        void setUser(User *user);
        User *user() const;
        
        void generateSign();
        
        void signPackage(QDomDocument &doc);
        
        QString sign() const;
        
        void setAuthenticationFlag(bool flag);
        bool isAuthenticated() const;
        QString ip() const;

    protected:
        void timerEvent(QTimerEvent *event);
        
    public slots:
        void close();
        void sendNotification(int code, const QString &text, Notification::Level level);
        
    private slots:
        void removeConnection();
        
    signals:
        void error(QTcpSocket::SocketError socketError);
        void requestSendToAll(const QString &msg);
        void connectionClosed(Connection *connection);
        void packageReaded(Connection *connection, const QString& root,const QString & packages);
        
    private:
        Client *m_client;
        TcpServer *m_server;
        QString m_ip;
        bool m_auth;
        QQueue<QString> m_readed;
        QHash<int, QVariant> m_datas;
        QString m_sign;
        User *m_user;
};

#endif
