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
#include "usermanager.h"

#include "user.h"
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

UserManager::UserManager(QObject *parent) : Observer()
{
    Q_UNUSED(parent)
    m_user = new User();
}

UserManager::~UserManager()
{
}

bool UserManager::userExists(const QString &username)
{
    QString sql = "SELECT count(*) FROM tupitube_user WHERE username='" + username + "'";
    QSqlQuery query(sql);
    QString one = "0";

    if (query.first())
        one = query.value(0).toString();

    query.clear();

    #ifdef TUP_DEBUG
           qDebug() << "[UserManager::userExists()] - SQL: " << sql;
    #endif

    if (one.compare("1") == 0)
        return true;

    return false;
}

bool UserManager::verifyPassword(const QString &username, const QString &passwd)
{
    QString sql = "SELECT user_id, name, password, is_enabled, is_creator, projects_public_policy, files_public_policy, works_public_policy FROM tupitube_user WHERE username='" + username + "'";
    QSqlQuery query(sql);

    #ifdef TUP_DEBUG
           qDebug() << "[UserManager::verifyPassword()] - SQL: " << sql;
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
                m_user = userData(uid, name, username, password, isEnabled, isCreator, projectsPrivacy, filesPrivacy, worksPrivacy);
                return true;
            }
        }
    }

    query.clear();

    return false;
}

User *UserManager::userData(int uid, const QString &name, const QString &username,
                            const QString &password, bool isEnabled, bool isCreator,
                            bool projectsPrivacy, bool filesPrivacy, bool worksPrivacy)
{
    User *user = new User();
    user->setUID(uid);
    user->setName(name);
    user->setLogin(username);
    user->setPassword(password);

    user->setEnabledFlag(isEnabled);
    user->setCreatorFlag(isCreator);
    user->setProjectsPrivacyFlag(projectsPrivacy);
    user->setFilesPrivacyFlag(filesPrivacy);
    user->setWorksPrivacyFlag(worksPrivacy);

    return user;
}

void UserManager::handlePackage(PackageBase *const pkg)
{
    #ifdef TUP_DEBUG
        qDebug() << "[UserManager::handlePackage()]";
    #endif

    QString root = pkg->root();
    QString package = pkg->xml();
    Connection *connection = pkg->source();
    QString ip = connection->client()->peerAddress().toString();
    
    if (root == "user_connect") {

        ConnectParser parser(package);

        if (parser.parse()) {
            QString username = parser.username();
            QString passwd = parser.password();

            if (username.length() > 0 && passwd.length() > 0) {
                if (userExists(username)){
                    if (verifyPassword(username, passwd)) {
                        if (!m_user->isEnabled() || !m_user->isCreator()) {
                            Logger::self()->error(QObject::tr("User \"%1\" is disabled to log in").arg(username));
                            #ifdef TUP_DEBUG
                                   qDebug() << "[UserManager::handlePackage()] - Error: User " << username << " is disabled to log in";
                            #endif

                            Ban ban(username, 0);
                            connection->sendStringToClient(ban, false);
                            return;
                        }

                        if (m_online.contains(username)) {
                            Notification error(400, QObject::tr("User is already logged on"), Notification::Error);
                            connection->sendStringToClient(error);
                            Logger::self()->error(QObject::tr("User is already logged on -> %1").arg(username));
                            connection->close();
                            return;
                        } else {
                            m_online << username;
                        }

                        if (parser.clientType() == 0) {
                            Logger::self()->info(QObject::tr("User \"%1\" has logged in from artist client [%2]").arg(username, ip));
                            emit userConnected(username, ip);
                        } else {
                            Logger::self()->error(QObject::tr("User \"%1\" has logged in from unknown client").arg(username));
                            #ifdef TUP_DEBUG
                                   qDebug() << "[UserManager::handlePackage()] - Connection denied!";
                            #endif
                            connection->close();
                            return;
                        }

                        connection->setUser(m_user);
                        Ack ack(connection->sign());
                        connection->sendStringToClient(ack, false);

                    } else {
                        Notification error(400, QObject::tr("Invalid username or password"), Notification::Error);
                        connection->sendStringToClient(error);
                        Logger::self()->error(QObject::tr("Invalid username or password -> %1").arg(username));
                        connection->close();
                    }
                } else {
                    Notification error(402, QObject::tr("User <b>%1</b> doesn't exist").arg(username), Notification::Error);
                    connection->sendStringToClient(error);
                    Logger::self()->error(QObject::tr("User doesn't exist -> %1").arg(username));
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
                   qDebug() << "[UserManager::handlePackage()] - Error: Null data within package coming from " << ip;
            #endif
            connection->close();
        }
    } 
}

void UserManager::closeConnection(Connection *connection)
{
    User *user = connection->user();
    if (user) {
        QString username = user->login();
        m_online.removeAll(username);
        emit userDisconnected(username);
    }
}
