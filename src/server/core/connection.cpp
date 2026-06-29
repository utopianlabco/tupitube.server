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
#include "../modules/students/student.h"
#include <QtNetwork>
#include <QCryptographicHash>
#include <QDebug>
#include <QCoreApplication> // Required for processEvents()

Connection::Connection(qintptr socketDescriptor, TcpServer *server)
    : QThread(server),
      m_socketDescriptor(socketDescriptor), // Matches header declaration order
      m_client(nullptr),
      m_server(server),
      m_student(nullptr)
{
#ifdef TUP_DEBUG
    qDebug() << "[Connection::Connection()]";
#endif
    m_ip = "unknown";
    m_auth = false;

    // DO NOT create m_client here. It is now created inside run() to fix Windows thread affinity.
}

Connection::~Connection()
{
    // m_client is safely deleted at the end of run()
    delete m_student;
}

void Connection::run()
{
#ifdef TUP_DEBUG
    qDebug() << "[Connection::run()]";
#endif

    // 1. Create the socket INSIDE the worker thread.
    // This ensures the OS socket handle belongs to this thread, fixing the Windows crash.
    m_client = new Client(this);

    if (!m_client->setSocketDescriptor(m_socketDescriptor)) {
        #ifdef TUP_DEBUG
           qWarning() << "[Connection::run()] - Error: " << m_client->error();
        #endif
        delete m_client;
        m_client = nullptr;
        return;
    }

    m_ip = m_client->peerAddress().toString();
    if (m_ip.isNull())
        m_ip = "unknown";

    while (m_client && m_client->state() != QAbstractSocket::UnconnectedState) {

       // CRITICAL: Process queued method calls (like sendStringToClient) from the main thread
       QCoreApplication::processEvents();

       if (m_readed.isEmpty()) {
           QThread::msleep(10); // Prevent 100% CPU usage in busy-wait loop
           continue;
       }

       if (!m_student)
           setAuthenticationFlag(false);

       if (!m_readed.isEmpty()) {
           QString package = QString::fromUtf8(m_readed.dequeue().toUtf8());

           if (!package.isNull()) {
               #ifdef TUP_DEBUG
                      qDebug()  <<  "*** [Connection::run()] - Package received:  ";
                      qWarning()  << package;
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

    // Cleanup socket before exiting thread
    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }

    removeConnection();
}

void Connection::removeConnection()
{
    emit connectionClosed(this);
}

void Connection::close()
{
    if (m_student && isAuthenticated())
        Logger::self()->info(QObject::tr("Student \"%1\" has logged off [%2]").arg(m_student->login(), m_ip));

    setAuthenticationFlag(false);
    m_readed.clear();

    // Safely invoke socket methods in the worker thread using QueuedConnection
    if (m_client) {
        QMetaObject::invokeMethod(m_client, "flush", Qt::QueuedConnection);
        QMetaObject::invokeMethod(m_client, "disconnectFromHost", Qt::QueuedConnection);
        QMetaObject::invokeMethod(m_client, "close", Qt::QueuedConnection);
    }
}

void Connection::appendTextReaded(const QString &package)
{
    m_readed.enqueue(QString::fromUtf8(package.toUtf8()));
}

void Connection::sendStringToClient(const QString &text)
{
    if (m_client) {
        QMetaObject::invokeMethod(m_client, "send", Qt::QueuedConnection, Q_ARG(QString, text));
    }
}

void Connection::sendFileToClient(const QString &path)
{
    if (m_client) {
        QMetaObject::invokeMethod(m_client, "sendFile", Qt::QueuedConnection, Q_ARG(QString, path));
    }
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

    if (m_client) {
        // Convert to QString to avoid QMetaType registration issues with QDomDocument
        QString xmlString = doc.toString(0);
        QMetaObject::invokeMethod(m_client, "send", Qt::QueuedConnection, Q_ARG(QString, xmlString));
    }
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

void Connection::setStudent(Student *student)
{
#ifdef TUP_DEBUG
    qDebug() << "[Connection::setStudent()]";
#endif
    m_student = student;
    generateSign();
    setAuthenticationFlag(true);
}

Student *Connection::student() const
{
    return m_student;
}

void Connection::generateSign()
{
    if (m_student) {
        QString input = m_student->login() + m_student->password() + TAlgorithm::randomString(TAlgorithm::random() % 10);
        QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5);
        m_sign = hash.toHex();
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
