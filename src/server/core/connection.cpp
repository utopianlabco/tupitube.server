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
#include "connection.h"

#include "talgorithm.h"
#include "server.h"
#include "logger.h"
#include "tuprequestparser.h"
#include "tupcompress.h"
#include "tupprojectrequest.h"
#include "tupprojectresponse.h"
#include "tuprequestparser.h"
#include "user.h"

#include <QtNetwork>
#include <QCryptographicHash>
#include <QDebug>

Connection::Connection(qintptr socketDescriptor, TcpServer *server) : QThread(server), m_server(server), m_user(nullptr)
{
    #ifdef TUP_DEBUG
        qDebug() << "[Connection::Connection()]";
    #endif

    m_client = new Client(this);

    if (!m_client->setSocketDescriptor(socketDescriptor)) {
        #ifdef TUP_DEBUG
           qWarning() << "[Connection::Connection()] - Error: " << m_client->error();
        #endif
        return;
    }

    m_ip = m_client->peerAddress().toString();
  
    if (m_ip.isNull())
        m_ip = "unknown";
}

Connection::~Connection()
{
    delete m_client;
    delete m_user;
}

void Connection::run()
{
    while (m_client->state() != QAbstractSocket::UnconnectedState) {
       if (m_readed.isEmpty())
           continue;

       if (!m_user)
           setAuthenticationFlag(false);

       if (!m_readed.isEmpty()) {
           QString package = QString::fromUtf8(m_readed.dequeue().toUtf8());

           // SQA: Code to trace tricky packages
           // qDebug("server") << "Connection::run() - package: ";
           // qDebug("server") << package;

           if (!package.isNull()) {
               #ifdef TUP_DEBUG
                      qDebug() << "*** [Connection::run()] - Package received: ";
                      qWarning() << package;
               #endif

               QDomDocument doc;

               if (doc.setContent(package.trimmed())) {
                   QString root = doc.documentElement().tagName();

                   if ((root.compare("user_connect") == 0 && !isAuthenticated()) || 
                       (root.compare("user_connect") != 0 && isAuthenticated())) {
                        emit packageReaded(this, root, package);
                   } else {
                        #ifdef TUP_DEBUG
                               qDebug() << "[Connection::run()] - Error: malicious package";
                        #endif
                        break;
                   }
               } else {
                   #ifdef TUP_DEBUG
                          qDebug() << "[Connection::run()] - Error: Incoming package is invalid - ip source -> " << m_ip;
                   #endif
                   break;
               }
           } else {
               #ifdef TUP_DEBUG
                      qDebug() << "[Connection::run()] - Error: Package is null!";
               #endif
               break;
           }
       }
    }

    // SQA: Code to trace tricky packages
    // qDebug("server") << "Connection::run() - Connection State == QAbstractSocket::UnconnectedState";

    removeConnection();
}

void Connection::removeConnection()
{
    emit connectionClosed(this);
}

void Connection::close()
{
    if (m_user && isAuthenticated())
        Logger::self()->info(QObject::tr("User \"%1\" has logged off [%2]").arg(m_user->login(), m_ip)); 

    setAuthenticationFlag(false);
    
    m_readed.clear();

    if (m_client->state() != QAbstractSocket::UnconnectedState) {
        m_client->flush();
        m_client->disconnectFromHost();
        m_client->close();
    }
}

void Connection::appendTextReaded(const QString &package)
{
    // SQA: Code to trace tricky packages 
    // qDebug("server") << "Connection::appendTextReaded() - Tracing data:";
    // qDebug("server") << package;

    m_readed.enqueue(QString::fromUtf8(package.toUtf8()));
}

void Connection::sendStringToClient(const QString &text)
{
    m_client->send(text);
}

void Connection::sendFileToClient(const QString &path)
{
    m_client->sendFile(path);
}

void Connection::setData(int key, const QVariant &value)
{
    m_datas.insert(key, value);
}

QVariant Connection::data(int key) const
{
    return m_datas[key];
}

Client *Connection::client() const
{
    return m_client;
}

TcpServer *Connection::server() const
{
    return m_server;
}

void Connection::sendToAll(const QString &text)
{
    emit requestSendToAll(text);
}

void Connection::sendStringToClient(QDomDocument &doc, bool sign)
{
    #ifdef TUP_DEBUG
        qDebug() << "[Connection::sendStringToClient()]";
    #endif

    if (sign)
        signPackage(doc);

    #ifdef TUP_DEBUG   
       qDebug() << "[Connection::sendStringToClient()] - Sending package: ";
       qWarning() << doc.toString();
    #endif
    
    m_client->send(doc);
}

void Connection::sendToAll(QDomDocument &doc, bool sign)
{
    if (sign)
        signPackage(doc);

    emit requestSendToAll(doc.toString(0));
}

void Connection::signPackage(QDomDocument &doc)
{
    doc.documentElement().setAttribute("sign", m_sign);
}

QString Connection::sign() const
{
    return m_sign;
}

void Connection::setUser(User *user)
{
    #ifdef TUP_DEBUG
       qDebug() << "[Connection::setUser()]";
    #endif

    m_user = user;
    generateSign();
    
    setAuthenticationFlag(true);
}

User *Connection::user() const
{
    return m_user;
}

void Connection::generateSign()
{
    if (m_user) {
        QString input = m_user->login() + m_user->password() + TAlgorithm::randomString(TAlgorithm::random() % 10);
        QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5);     
        m_sign = hash.toHex();
        // m_sign = TMD5Hash::hash(m_user->login() + m_user->password() + TAlgorithm::randomString(TAlgorithm::random() % 10));
    }
}

void Connection::sendNotification(int code, const QString &text, Notification::Level level)
{
    Notification message(code, text, level);
    sendStringToClient(message);
}

void Connection::setAuthenticationFlag(bool flag)
{
    m_auth = flag;
}

bool Connection::isAuthenticated() const
{
    return m_auth;
}

QString Connection::ip() const
{
    return m_ip;
}

void Connection::timerEvent(QTimerEvent *event) 
{
    Q_UNUSED(event);

    #ifdef TUP_DEBUG
        qDebug() << "*** [Connection::timerEvent()] - Connection closed by inactivity from -> " << m_ip;
    #endif

    emit connectionClosed(this);
}
