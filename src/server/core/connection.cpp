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
#include <QMutexLocker>

Connection::Connection(qintptr socketDescriptor, TcpServer *server)
    : QThread(server),
      m_socketDescriptor(socketDescriptor),
      m_client(nullptr),
      m_server(server),
      m_ip("unknown"),
      m_auth(false),
      m_shouldClose(false),
      m_student(nullptr),
      m_inactivityTimerId(0),
      m_inactivityTimeoutMs(600000) // Default: 10 minutes
{
#ifdef TUP_DEBUG
    qDebug() << "[Connection::Connection()]";
#endif
}

Connection::~Connection()
{
    delete m_student;
}

void Connection::run()
{
#ifdef TUP_DEBUG
    qDebug() << "[Connection::run()]";
#endif

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

    // ✅ Start the inactivity timer inside the worker thread
    resetInactivityTimer();

    while (m_client && m_client->state() != QAbstractSocket::UnconnectedState) {

       {
           QMutexLocker locker(&m_closeMutex);
           if (m_shouldClose) {
               m_client->flush();
               m_client->disconnectFromHost();
               m_client->close();
               break;
           }
       }

       if (m_client->waitForReadyRead(100)) {
           QByteArray data = m_client->readAll();
           appendTextReaded(QString::fromUtf8(data));
       }

       {
           QMutexLocker locker(&m_sendMutex);
           while (!m_sendQueue.isEmpty()) {
               QString text = m_sendQueue.dequeue();
               if (text.startsWith("FILE:")) {
                   m_client->sendFile(text.mid(5));
               } else {
                   m_client->send(text);
               }
           }
       }

       {
           QMutexLocker locker(&m_readedMutex);
           while (!m_readed.isEmpty()) {
               QString package = m_readed.dequeue();

               if (!m_student)
                   setAuthenticationFlag(false);

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
               }
           }
       }
    }

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
    if (m_student && isAuthenticated()) {
        if (m_disconnectedByInactivity) {
           Logger::self()->info(QObject::tr("Student \"%1\" has been disconnected due to inactivity [%2]").arg(m_student->login(), m_ip));
        } else {
           Logger::self()->info(QObject::tr("Student \"%1\" has logged off [%2]").arg(m_student->login(), m_ip));
        }
    }

    setAuthenticationFlag(false);

    {
        QMutexLocker locker(&m_readedMutex);
        m_readed.clear();
    }

    {
        QMutexLocker locker(&m_closeMutex);
        m_shouldClose = true;
    }
}

void Connection::appendTextReaded(const QString &package)
{
    QMutexLocker locker(&m_readedMutex);
    m_readed.enqueue(package);

    // ✅ Reset the inactivity timer when data is received
    QMetaObject::invokeMethod(this, "resetInactivityTimer", Qt::QueuedConnection);
}

void Connection::sendStringToClient(const QString &text)
{
    QMutexLocker locker(&m_sendMutex);
    m_sendQueue.enqueue(text);
}

void Connection::sendFileToClient(const QString &path)
{
    QMutexLocker locker(&m_sendMutex);
    m_sendQueue.enqueue("FILE:" + path);
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

    QString xmlString = doc.toString(0);
    QMutexLocker locker(&m_sendMutex);
    m_sendQueue.enqueue(xmlString);
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
    sendStringToClient(message.toString());
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
    if (event->timerId() == m_inactivityTimerId) {
        #ifdef TUP_DEBUG
                qDebug() << "** [Connection::timerEvent()] - Connection closed by inactivity from -> " << m_ip;
        #endif
        m_disconnectedByInactivity = true;
        // Creating the inactivity disconnect package
        QDomDocument doc;
        QDomElement root = doc.createElement("disconnect");
        root.setAttribute("reason", "inactivity");
        doc.appendChild(root);

        // Queueing it to be sent to the client
        sendStringToClient(doc);

        // Triggering graceful shutdown
        // The run() loop will see m_shouldClose=true, flush the socket (sending our package), and close.
        close();
    } else {
        QThread::timerEvent(event);
    }
}

void Connection::setInactivityTimeout(int timeoutMs)
{
    m_inactivityTimeoutMs = timeoutMs;

    if (m_inactivityTimerId != 0) {
        killTimer(m_inactivityTimerId);
        m_inactivityTimerId = startTimer(m_inactivityTimeoutMs);
    }
}

void Connection::resetInactivityTimer()
{
    if (m_inactivityTimerId != 0) {
        killTimer(m_inactivityTimerId);
    }
    m_inactivityTimerId = startTimer(m_inactivityTimeoutMs);

    #ifdef TUP_DEBUG
        qDebug() << "** [Connection::resetInactivityTimer()] - Timer reset for" << m_ip;
    #endif
}
