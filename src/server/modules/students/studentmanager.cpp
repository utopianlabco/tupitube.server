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
#include "studentmanager.h"

#include "student.h"
#include "packagebase.h"
#include "settings.h"
#include "logger.h"

#include "server.h"
#include "connection.h"
#include "connectparser.h"
#include "ack.h"
#include "ban.h"

#include <QHash>
#include <QDomDocument>
#include <QDebug>

StudentManager::StudentManager(QObject *parent) : Observer()
{
    Q_UNUSED(parent)
    m_student = new Student();
}

StudentManager::~StudentManager()
{
}

bool StudentManager::studentExists(const QString &studentname)
{
    QString sql = "SELECT count(*) FROM tupitube_student WHERE studentname='" + studentname + "'";
    QSqlQuery query(sql);
    QString one = "0";

    if (query.first())
        one = query.value(0).toString();

    query.clear();

    #ifdef TUP_DEBUG
           qDebug() << "[StudentManager::studentExists()] - SQL: " << sql;
    #endif

    if (one.compare("1") == 0)
        return true;

    return false;
}

bool StudentManager::verifyPassword(const QString &studentname, const QString &passwd)
{
    QString sql = "SELECT student_id, name, password, is_enabled, is_creator, projects_public_policy, files_public_policy, works_public_policy FROM tupitube_student WHERE studentname='" + studentname + "'";
    QSqlQuery query(sql);

    #ifdef TUP_DEBUG
           qDebug() << "[StudentManager::verifyPassword()] - SQL: " << sql;
    #endif 

    if (query.first()) {
        int uid = query.value(0).toInt();
        QString name = query.value(1).toString();
        QString password = query.value(2).toString();
        bool isEnabled = query.value(3).toInt() ? true : false;
        bool isCreator = query.value(4).toInt() ? true : false;
        bool projectsPrivacy = query.value(5).toInt() ? true : false;
        bool filesPrivacy = query.value(6).toInt() ? true : false;
        bool worksPrivacy = query.value(7).toInt() ? true : false;

        if (password.length() > 0) {
            QString cleanPass = passwd.mid(4);
            if (cleanPass.compare(password) == 0) {
                m_student = studentData(uid, name, studentname, password, isEnabled, isCreator, projectsPrivacy, filesPrivacy, worksPrivacy);
                return true;
            }
        }
    }

    query.clear();

    return false;
}

Student *StudentManager::studentData(int uid, const QString &name, const QString &studentname,
                            const QString &password, bool isEnabled, bool isCreator,
                            bool projectsPrivacy, bool filesPrivacy, bool worksPrivacy)
{
    Student *student = new Student();
    student->setUID(uid);
    student->setName(name);
    student->setLogin(studentname);
    student->setPassword(password);

    student->setEnabledFlag(isEnabled);
    student->setCreatorFlag(isCreator);
    student->setProjectsPrivacyFlag(projectsPrivacy);
    student->setFilesPrivacyFlag(filesPrivacy);
    student->setWorksPrivacyFlag(worksPrivacy);

    return student;
}

void StudentManager::handlePackage(PackageBase *const pkg)
{
    #ifdef TUP_DEBUG
        qDebug() << "[StudentManager::handlePackage()]";
    #endif

    QString root = pkg->root();
    QString package = pkg->xml();
    Connection *connection = pkg->source();
    QString ip = connection->client()->peerAddress().toString();
    
    if (root == "user_connect") {

        ConnectParser parser(package);

        if (parser.parse()) {
            QString studentname = parser.studentname();
            QString passwd = parser.password();

            if (studentname.length() > 0 && passwd.length() > 0) {
                if (studentExists(studentname)){
                    if (verifyPassword(studentname, passwd)) {
                        if (!m_student->isEnabled() || !m_student->isCreator()) {
                            Logger::self()->error(QObject::tr("Student \"%1\" is disabled to log in").arg(studentname));
                            #ifdef TUP_DEBUG
                                   qDebug() << "[StudentManager::handlePackage()] - Error: Student " << studentname << " is disabled to log in";
                            #endif

                            Ban ban(studentname, 0);
                            connection->sendStringToClient(ban, false);
                            return;
                        }

                        if (m_online.contains(studentname)) {
                            Notification error(400, QObject::tr("Student is already logged on"), Notification::Error);
                            connection->sendStringToClient(error);
                            Logger::self()->error(QObject::tr("Student is already logged on -> %1").arg(studentname));
                            connection->close();
                            return;
                        } else {
                            m_online << studentname;
                        }

                        if (parser.clientType() == 0) {
                            Logger::self()->info(QObject::tr("Student \"%1\" has logged in from artist client [%2]").arg(studentname, ip));
                            emit studentConnected(studentname, ip);
                        } else {
                            Logger::self()->error(QObject::tr("Student \"%1\" has logged in from unknown client").arg(studentname));
                            #ifdef TUP_DEBUG
                                   qDebug() << "[StudentManager::handlePackage()] - Connection denied!";
                            #endif
                            connection->close();
                            return;
                        }

                        connection->setStudent(m_student);
                        Ack ack(connection->sign());
                        connection->sendStringToClient(ack, false);

                    } else {
                        Notification error(400, QObject::tr("Invalid studentname or password"), Notification::Error);
                        connection->sendStringToClient(error);
                        Logger::self()->error(QObject::tr("Invalid studentname or password -> %1").arg(studentname));
                        connection->close();
                    }
                } else {
                    Notification error(402, QObject::tr("Student <b>%1</b> doesn't exist").arg(studentname), Notification::Error);
                    connection->sendStringToClient(error);
                    Logger::self()->error(QObject::tr("Student doesn't exist -> %1").arg(studentname));
                    connection->close();
                }
            } else {
                Notification error(400, QObject::tr("Network error: Corrupted Package!"), Notification::Error);
                connection->sendStringToClient(error);
                Logger::self()->error(QObject::tr("Network error: Corrupted Package!"));
                connection->close();
            }
        } else {
            Logger::self()->error(QObject::tr("Null data within package coming from %1").arg(ip));
            #ifdef TUP_DEBUG
                   qDebug() << "[StudentManager::handlePackage()] - Error: Null data within package coming from " << ip;
            #endif
            connection->close();
        }
    } 
}

void StudentManager::closeConnection(Connection *connection)
{
    Student *student = connection->student();
    if (student) {
        QString studentname = student->login();
        m_online.removeAll(studentname);
        emit studentDisconnected(studentname, connection->disconnectedByInactivity());
    }
}
