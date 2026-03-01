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
#ifndef USERMANAGER_H_
#define USERMANAGER_H_

#include "observer.h"

#include <QString>
#include <QtSql>

class UserManager : public Observer
{
    Q_OBJECT

    public:
        explicit UserManager(QObject *parent = nullptr);
        ~UserManager();

        void handlePackage(PackageBase *const pkg);
        void closeConnection(Connection *connection);

    signals:
        void userConnected(const QString &username, const QString &ip);
        void userDisconnected(const QString &username);
        
    private:
        bool verifyPassword(const QString &username, const QString &password);
        bool userExists(const QString &username);
        User *userData(int uid, const QString &name, const QString &username, const QString &password, bool isEnabled, 
                       bool isCreator, bool projectsPrivacy, bool filesPrivacy, bool worksPrivacy);

        User *m_user;
        QList<QString> m_online;
};

#endif
