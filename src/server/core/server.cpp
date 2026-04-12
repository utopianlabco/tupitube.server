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

    initDataBase();

    m_studentManager = new StudentManager(this);
    connect(m_studentManager, &StudentManager::studentConnected, this, &TcpServer::studentConnected);
    connect(m_studentManager, &StudentManager::studentDisconnected, this, &TcpServer::studentDisconnected);
    m_observers << m_studentManager;

    m_projectManager = new ProjectManager;
    m_observers << m_projectManager;
    connect(m_projectManager, &ProjectManager::projectEventLog, this, [this](const QString &msg, const QString &level) {
        emit logMessage(msg, level);
    });

    m_communicationManager = new CommunicationManager;
    m_observers << m_communicationManager;


}

TcpServer::~TcpServer()
{
    m_db.close();
    Logger::self()->info("Server finished");
    delete Settings::self();
    delete Logger::self();
    
    qDeleteAll(m_observers);
}

void TcpServer::initDataBase()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TcpServer::initDataBase()] - DB Drivers: " << QSqlDatabase::drivers();
    #endif

    TCONFIG->beginGroup("Database");
    QString driver = TCONFIG->value("Driver").toString();
    m_db = QSqlDatabase::addDatabase(driver);

    if (driver == "QSQLITE") {
        // SQLite: Get database path and name
        QString dbDir = TCONFIG->value("DatabasePath").toString();
        QString dbName = TCONFIG->value("DbName", "tupitube.db").toString();
        QString dbPath = dbDir + "/" + dbName;
        
        // Create directory if it doesn't exist
        QDir dir(dbDir);
        if (!dir.exists()) {
            #ifdef TUP_DEBUG
                qDebug() << "[TcpServer::initDataBase()] - Creating database directory:" << dbDir;
            #endif
            if (!dir.mkpath(".")) {
                qCritical() << "[TcpServer::initDataBase()] - Failed to create database directory:" << dbDir;
                exit(1);
            }
        }
        
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::initDataBase()] - Database path:" << dbPath;
        #endif
        m_db.setDatabaseName(dbPath);
    } else {
        // MySQL/PostgreSQL: use host, port, credentials
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

    // Check if tables exist, create them if not
    QStringList tables = m_db.tables();
    if (!tables.contains("tupitube_student")) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::initDataBase()] - Creating database tables...";
        #endif
        createDatabaseSchema();
    }

    TCONFIG->endGroup(); // Database
}

void TcpServer::createDatabaseSchema()
{
    QSqlQuery query(m_db);
    
    // Create class table
    QString createClassTable =
        "CREATE TABLE IF NOT EXISTS class ("
        "class_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name VARCHAR(50) NOT NULL,"
        "year INTEGER NOT NULL,"
        "description TEXT"
        ")";
    if (!query.exec(createClassTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating class:" << query.lastError().text();
        #endif
    }

    // Create period table
    QString createPeriodTable =
        "CREATE TABLE IF NOT EXISTS period ("
        "period_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name VARCHAR(50) NOT NULL,"
        "year INTEGER NOT NULL,"
        "start_date DATE,"
        "end_date DATE"
        ")";
    if (!query.exec(createPeriodTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating period:" << query.lastError().text();
        #endif
    }

    // Create tupitube_student table (updated: uses class_id)
    QString createStudentTable =
        "CREATE TABLE IF NOT EXISTS tupitube_student ("
        "student_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name VARCHAR(100),"
        "studentname VARCHAR(50) NOT NULL UNIQUE,"
        "password VARCHAR(255) NOT NULL,"
        "is_enabled INTEGER DEFAULT 1,"
        "is_creator INTEGER DEFAULT 1,"
        "projects_public_policy INTEGER DEFAULT 0,"
        "files_public_policy INTEGER DEFAULT 0,"
        "works_public_policy INTEGER DEFAULT 0,"
        "class_id INTEGER NOT NULL,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (class_id) REFERENCES class(class_id)"
        ")";
    if (!query.exec(createStudentTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating tupitube_student:" << query.lastError().text();
        #endif
    }

    // Create tupitube_project table (updated: class_id, period_id, group_project)
    QString createProjectTable =
        "CREATE TABLE IF NOT EXISTS tupitube_project ("
        "project_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title VARCHAR(100) NOT NULL,"
        "description TEXT,"
        "student_id INTEGER NOT NULL,"
        "filename VARCHAR(255) NOT NULL,"
        "is_public INTEGER DEFAULT 0,"
        "is_shared INTEGER DEFAULT 0,"
        "class_id INTEGER NOT NULL,"
        "period_id INTEGER NOT NULL,"
        "group_project INTEGER DEFAULT 0,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id),"
        "FOREIGN KEY (class_id) REFERENCES class(class_id),"
        "FOREIGN KEY (period_id) REFERENCES period(period_id)"
        ")";
    if (!query.exec(createProjectTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating tupitube_project:" << query.lastError().text();
        #endif
    }

    // Create project_student join table
    QString createProjectStudentTable =
        "CREATE TABLE IF NOT EXISTS project_student ("
        "project_id INTEGER NOT NULL,"
        "student_id INTEGER NOT NULL,"
        "PRIMARY KEY (project_id, student_id),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id)"
        ")";
    if (!query.exec(createProjectStudentTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating project_student:" << query.lastError().text();
        #endif
    }

    // Create tupitube_collection table
    QString createCollectionTable =
        "CREATE TABLE IF NOT EXISTS tupitube_collection ("
        "collection_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_id INTEGER,"
        "type VARCHAR(20),"
        "title VARCHAR(100),"
        "topics TEXT,"
        "description TEXT,"
        "student_id INTEGER NOT NULL,"
        "is_public INTEGER DEFAULT 0,"
        "visits INTEGER DEFAULT 0,"
        "likes INTEGER DEFAULT 0,"
        "project_id INTEGER,"
        "path VARCHAR(255),"
        "slug VARCHAR(100),"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (parent_id) REFERENCES tupitube_collection(collection_id)"
        ")";
    if (!query.exec(createCollectionTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating tupitube_collection:" << query.lastError().text();
        #endif
    }

    // Create tupitube_work table
    QString createWorkTable =
        "CREATE TABLE IF NOT EXISTS tupitube_work ("
        "work_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id INTEGER,"
        "project_id INTEGER,"
        "collection_id INTEGER,"
        "type_id INTEGER,"
        "type VARCHAR(20),"
        "title VARCHAR(100),"
        "content TEXT,"
        "topics TEXT,"
        "tags TEXT,"
        "description TEXT,"
        "filename VARCHAR(255),"
        "is_public INTEGER DEFAULT 0,"
        "enabled INTEGER DEFAULT 0,"
        "visits INTEGER DEFAULT 0,"
        "duration REAL DEFAULT 0,"
        "portrait INTEGER DEFAULT 0,"
        "mobile INTEGER DEFAULT 0,"
        "rendered INTEGER DEFAULT 0,"
        "uploaded INTEGER DEFAULT 0,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "updated_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "FOREIGN KEY (collection_id) REFERENCES tupitube_collection(collection_id)"
        ")";
    if (!query.exec(createWorkTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating tupitube_work:" << query.lastError().text();
        #endif
    }

    // Create tupitube_collaboration table (unchanged)
    QString createCollabTable =
        "CREATE TABLE IF NOT EXISTS tupitube_collaboration ("
        "collaboration_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id INTEGER NOT NULL,"
        "project_id INTEGER NOT NULL,"
        "permission_level INTEGER DEFAULT 1,"
        "created_at DATETIME DEFAULT (datetime('now')),"
        "FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id),"
        "FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),"
        "UNIQUE(student_id, project_id)"
        ")";
    if (!query.exec(createCollabTable)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TcpServer::createDatabaseSchema()] - Error creating tupitube_collaboration:" << query.lastError().text();
        #endif
    }
    
    #ifdef TUP_DEBUG
        qDebug() << "[TcpServer::createDatabaseSchema()] - Database schema created successfully";
    #endif
}

bool TcpServer::openConnection(const QString &host, int port)
{
    QList<QHostAddress> address = QHostInfo::fromName(host).addresses();
    if (!address.isEmpty()) {
        if (!listen(QHostAddress::Any, port)) {
            #ifdef TUP_DEBUG
                   qDebug() << "[TcpServer::openConnection()] - Error: Can't connect to " << host << ":" << port;
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
	qDebug() << "";
	qDebug() << ("" + record);
        qWarning() << "[TcpServer::incomingConnection()] - Handling connection #" << m_connections.count();
        qDebug() << "[TcpServer::incomingConnection()] - New connection detected!";
    #endif

    Connection *newConnection = new Connection(socketDescriptor, this);
    if (newConnection) {
        QString ip = newConnection->client()->peerAddress().toString();
        handle(newConnection);
        m_connections << newConnection;
        // newConnection->startTimer(60000); // 1 minute
        newConnection->startTimer(600000); // 10 minutes
        newConnection->start();

        emit connectionCountChanged(m_connections.count());
        // emit logMessage(QObject::tr("New connection from %1").arg(ip), "INFO");
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

    connect(connection, SIGNAL(finished()), connection, SLOT(deleteLater()));
    connect(connection, SIGNAL(requestSendToAll(const QString&)), 
            this, SLOT(sendToAll(const QString&)));
    connect(connection, SIGNAL(packageReaded(Connection*, const QString&, const QString&)),
            this, SLOT(handlePackage(Connection*, const QString&, const QString&)));
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

    m_connections.removeAll(connection);
    m_managers.removeAll(connection);

    emit connectionCountChanged(m_connections.count());

    connection->blockSignals(true);
    connection->close();
    connection->blockSignals(false);
    
    foreach (Observer *observer, m_observers)
        observer->closeConnection(connection);

    if (!connection->isRunning()) {
        #ifdef TUP_DEBUG
               qWarning() << "[TcpServer::removeConnection()] - Deleting connection pointer";
        #endif

        delete connection;
        connection = 0;
    }

    #ifdef TUP_DEBUG
        qDebug() << "";
        qDebug() << "***";
    #endif
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
    }

    // delete pkg;
}
