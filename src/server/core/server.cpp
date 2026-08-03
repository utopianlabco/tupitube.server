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

#include "server.h"
#include "connection.h"
#include "tconfig.h"
#include "tapplicationproperties.h"
#include "packagehandlerbase.h"
#include "defaultpackagehandler.h"
#include "../modules/projects/projectmanager.h"
#include "../modules/students/studentmanager.h"
#include "communicationmanager.h"
#include "settings.h"
#include "package.h"
#include "observer.h"
#include "logger.h"
#include "connectparser.h"
#include "../modules/students/ack.h"
#include "../modules/students/ban.h"
#include <QHostInfo>
#include <QTimer>
#include <QQueue>
#include <QHostAddress>
#include <QSqlDatabase>
#include <QtSql>
#include <QDir>
#include <QDate>
#include <QTime>

TcpServer::TcpServer(QObject *parent) : QTcpServer(parent)
{
#ifdef TUP_DEBUG
    qDebug() << "[TcpServer::TcpServer()]";
#endif
#ifndef TUPITUBE_TEST
    initDataBase();
#endif

    m_studentManager = new StudentManager(this);
    connect(m_studentManager, &StudentManager::studentConnected, this, &TcpServer::studentConnected);
    connect(m_studentManager, &StudentManager::studentDisconnected, this, &TcpServer::studentDisconnected);
    m_observers << m_studentManager;

    m_projectManager = new ProjectManager;
    m_observers << m_projectManager;
    connect(m_projectManager, &ProjectManager::projectEventLog, this, [this](const QString &msg, const QString &level) {
        emit logMessage(msg, level);
    });
    connect(m_projectManager, &ProjectManager::projectRegistered, this, &TcpServer::projectRegistered);

    m_communicationManager = new CommunicationManager;
    m_observers << m_communicationManager;
}

TcpServer::~TcpServer()
{
#ifdef TUP_DEBUG
    qWarning() << "[TcpServer::~TcpServer()]";
#endif

    shutdownConnections();

    qDeleteAll(m_observers);
    m_observers.clear();

    m_studentManager = nullptr;
    m_projectManager = nullptr;
    m_communicationManager = nullptr;

    m_db.close();

    Logger::self()->info("Server finished");
    Settings::reset();
    Logger::reset();
}

void TcpServer::shutdownConnections()
{
#ifdef TUP_DEBUG
    qWarning()
        << "[TcpServer::shutdownConnections()]"
        << "Stopping active connections:"
        << m_connections.count();
#endif

    // Stop accepting new clients before shutting down existing workers.
    close();

    const QList<Connection *> connections = m_connections;

    // Prevent connectionClosed() from invoking removeConnection() while this
    // method is already performing deterministic shutdown.
    for (Connection *connection : connections) {
        if (!connection)
            continue;

        connection->blockSignals(true);
        connection->close();
    }

    // Release project, student, and communication state while the observers
    // still exist and while each Connection object is still valid.
    for (Connection *connection : connections) {
        if (!connection)
            continue;

        for (Observer *observer : m_observers) {
            if (observer)
                observer->closeConnection(connection);
        }
    }

    for (Connection *connection : connections) {
        if (!connection)
            continue;

        if (connection->isRunning() && !connection->wait(5000)) {
            qCritical()
                << "[TcpServer::shutdownConnections()]"
                << "Connection thread did not stop cleanly:"
                << connection->ip();

            // Emergency fallback only. The normal path exits through the
            // m_shouldClose flag checked by Connection::run().
            connection->terminate();
            connection->wait();
        }

        delete connection;
    }

    m_connections.clear();
    m_managers.clear();

    emit connectionCountChanged(0);

#ifdef TUP_DEBUG
    qWarning()
        << "[TcpServer::shutdownConnections()]"
        << "All connection threads stopped.";
#endif
}

void TcpServer::initDataBase()
{
    TCONFIG->beginGroup("Database");
    QString driver = TCONFIG->value("Driver").toString();
    m_db = QSqlDatabase::addDatabase(driver);
#ifdef TUP_DEBUG
    qDebug() << "[TcpServer::initDataBase()] - Config file path: " << TCONFIG->configPath();
    qDebug() << "[TcpServer::initDataBase()] - DB Drivers: " << QSqlDatabase::drivers();
#endif

    if (driver == "QSQLITE") {
        QString dbDir = TCONFIG->value("DatabasePath").toString();
        QString dbName = TCONFIG->value("DbName", "tupitube.db").toString();
        QString dbPath = dbDir + "/" + dbName;

        QDir dir(dbDir);
        if (!dir.exists()) {
#ifdef TUP_DEBUG
            qDebug() << "[TcpServer::initDataBase()] - Creating database directory: " << dbDir;
#endif
            if (!dir.mkpath(".")) {
                qCritical() << "[TcpServer::initDataBase()] - Failed to create database directory: " << dbDir;
                exit(1);
            }
        }

#ifdef TUP_DEBUG
        qDebug() << "[TcpServer::initDataBase()] - Using DB driver: " << driver;
        qDebug() << "[TcpServer::initDataBase()] - Using DB path: " << dbPath;
#endif
        m_db.setDatabaseName(dbPath);
    } else {
        m_db.setHostName(TCONFIG->value("Host").toString());
        m_db.setPort(TCONFIG->value("Port").toInt());
        m_db.setDatabaseName(TCONFIG->value("DbName").toString());
        m_db.setUserName(TCONFIG->value("Student").toString());
        m_db.setPassword(TCONFIG->value("Password").toString());
        if (driver == "QMYSQL")
            m_db.setConnectOptions("MYSQL_OPT_RECONNECT=1");
    }

    bool ok = m_db.open();
    if (!ok) {
        QSqlError error = m_db.lastError();
#ifdef TUP_DEBUG
        qDebug() << "[TcpServer::initDataBase()] - Fatal Error: Cannot connect to DB server...";
        qDebug() << "[TcpServer::initDataBase()] - Description: " << error.text();
#endif
        exit(1);
    }

    if (driver == "QSQLITE") {
        QSqlQuery pragmaQuery(m_db);
        pragmaQuery.exec("PRAGMA foreign_keys = ON;");

        QSqlQuery fkQuery(m_db);
        fkQuery.exec("PRAGMA foreign_key_list('tupitube_project')");
        bool foundRestrict = false;
        while (fkQuery.next()) {
            QString from = fkQuery.value(3).toString();
            QString to = fkQuery.value(4).toString();
            QString onDelete = fkQuery.value(6).toString();
            if (from == "student_id" && to == "student_id" && onDelete.toUpper() == "RESTRICT") {
                foundRestrict = true;
                break;
            }
        }
        if (!foundRestrict) {
            qWarning() << "[TcpServer::initDataBase()] - WARNING: Foreign key constraint on tupitube_project.student_id is not ON DELETE RESTRICT. Project ownership integrity is NOT enforced!";
        }
    }

    if (driver == "QSQLITE") {
        QStringList tables = m_db.tables();
        if (tables.contains("tupitube_project")) {
            QSqlQuery fkQuery(m_db);
            fkQuery.exec("PRAGMA foreign_key_list('tupitube_project')");
            bool foundRestrict = false;
            while (fkQuery.next()) {
                QString from = fkQuery.value(3).toString();
                QString to = fkQuery.value(4).toString();
                QString onDelete = fkQuery.value(6).toString();
                if (from == "student_id" && to == "student_id" && onDelete.toUpper() == "RESTRICT") {
                    foundRestrict = true;
                    break;
                }
            }
            if (!foundRestrict) {
                qWarning() << "[TcpServer::initDataBase()] - WARNING: Foreign key constraint on tupitube_project.student_id is not ON DELETE RESTRICT. Project ownership integrity is NOT enforced!";
            }
        }
    }

    TCONFIG->endGroup();
}

bool TcpServer::openConnection(const QString &host, int port)
{
    QList<QHostAddress> address = QHostInfo::fromName(host).addresses();
    if (!address.isEmpty()) {
        if (!listen(QHostAddress::Any, port)) {
#ifdef TUP_DEBUG
            qDebug() << "[TcpServer::openConnection()] - Error: Can't connect to " << host << ": " << port;
            qDebug() << "[TcpServer::openConnection()] - Description: " << errorString();
#endif
            return false;
        }
    } else {
#ifdef TUP_DEBUG
        qDebug() << "[TcpServer::openConnection()] - Error: Can't resolve address " << host;
#endif
        return false;
    }

    Logger::self()->info(QObject::tr("TupiTube media server started on %1:%2").arg(host).arg(port));
    return true;
}

void TcpServer::addAdmin(Connection *connection)
{
    m_managers << connection;
}

StudentManager *TcpServer::studentManager() const
{
    return m_studentManager;
}

int TcpServer::connectionCount() const
{
    return m_connections.count();
}

void TcpServer::addObserver(Observer *observer)
{
    m_observers << observer;
}

bool TcpServer::removeObserver(Observer *observer)
{
    return m_observers.removeAll(observer) > 0;
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    #ifdef TUP_DEBUG
        QDate date = QDate::currentDate();
        QTime time = QTime::currentTime();
        QString today = date.toString("yyyy-MM-dd");
        QString now = time.toString("hh:mm:ss");
        QString record = "[" + today + " " + now + "]";
        qDebug() << " ";
        qDebug() << record;
        qWarning() << "[TcpServer::incomingConnection()] - Handling connection # " << m_connections.count();
        qDebug() << "[TcpServer::incomingConnection()] - New connection detected!";
    #endif

    // ✅ Read the configurable timeout from the "Connection" group
    TCONFIG->beginGroup("Connection");
    int timeoutMinutes = TCONFIG->value("InactivityTimeout", 10).toInt();
    TCONFIG->endGroup();

    // Convert minutes to milliseconds
    int timeoutMs = timeoutMinutes * 60 * 1000;

    Connection *newConnection = new Connection(socketDescriptor, this);
    if (newConnection) {
        handle(newConnection);
        m_connections << newConnection;

        // ✅ Apply the configurable timeout
        newConnection->setInactivityTimeout(timeoutMs);

        // Start the thread (the timer will be initialized inside run())
        newConnection->start();

        emit connectionCountChanged(m_connections.count());
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::incomingConnection()] - Fatal Error: while setting connection";
        #endif
    }
}

void TcpServer::handle(Connection *connection)
{
#ifdef TUP_DEBUG
    qDebug() << "[TcpServer::handle()] - Processing new connection";
#endif

    connect(connection, SIGNAL(requestSendToAll(const QString &)),
            this, SLOT(sendToAll(const QString &)));

    connect(connection, SIGNAL(packageReaded(Connection*, const QString &, const QString &)),
            this, SLOT(handlePackage(Connection*, const QString &, const QString &)));

    connect(connection, SIGNAL(connectionClosed(Connection*)),
            this, SLOT(removeConnection(Connection*)));
}

void TcpServer::sendToAll(const QString &msg)
{
    foreach (Connection *connection, m_connections)
        connection->sendStringToClient(msg);
}

void TcpServer::sendToAll(const QDomDocument &pkg)
{
#ifdef TUP_DEBUG
    qDebug() << "[TcpServer::sendToAll()]";
#endif
    sendToAll(pkg.toString(0));
}

void TcpServer::sendToAdmins(const QString &message)
{
#ifdef TUP_DEBUG
    qDebug() << "[TcpServer::sendToAdmins()] - Sending:";
    qWarning() << message;
#endif
    foreach (Connection *connection, m_managers)
        connection->sendStringToClient(message);
}

void TcpServer::removeConnection(Connection *connection)
{
#ifdef TUP_DEBUG
    qDebug() << "[TcpServer::removeConnection()]";
#endif

    connection->blockSignals(true);

    m_connections.removeAll(connection);
    m_managers.removeAll(connection);

    emit connectionCountChanged(m_connections.count());

    connection->close();

    foreach (Observer *observer, m_observers)
        observer->closeConnection(connection);

    connection->wait(5000);

#ifdef TUP_DEBUG
    qWarning() << "[TcpServer::removeConnection()] - Deleting connection pointer";
#endif

    delete connection;
}

void TcpServer::handlePackage(Connection* client, const QString &root, const QString &package)
{
#ifdef TUP_DEBUG
    qDebug() << "[TcpServer::handlePackage()]";
#endif
    PackageBase *pkg = new PackageBase(root, package, client);

    if (root.startsWith("project_")) {
        m_projectManager->handlePackage(pkg);
    } else if (root.compare("user_connect") == 0) {
        m_studentManager->handlePackage(pkg);
    } else if (root.startsWith("communication_")) {
        m_communicationManager->handlePackage(pkg);
    } else if (root.compare("ping") == 0) {
        delete pkg;
    }
}
