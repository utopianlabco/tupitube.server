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
#ifndef USER_H
#define USER_H

#include <QString>
#include <QList>

class QDomDocument;
class QDomElement;

class User
{
    public:
        explicit User();
        ~User();

        void setUID(int uid);        
        void setName(const QString &name);
        void setLogin(const QString &login);
        void setPassword(const QString &password);
        void setEnabledFlag(bool flag);
        void setCreatorFlag(bool flag);
        void setProjectsPrivacyFlag(bool flag);
        void setFilesPrivacyFlag(bool flag);
        void setWorksPrivacyFlag(bool flag);
       
        int uid(); 
        QString name() const;
        QString login() const;
        QString password() const;
        bool isCreator();
        bool isEnabled();
        bool projectsPrivacyFlag();
        bool filesPrivacyFlag();
        bool worksPrivacyFlag();
        
        bool operator==(const User& user);
        
    private:
        int m_uid;
        QString m_name;
        QString m_login;
        QString m_password;
        bool m_isEnabled;
        bool m_isCreator;
        bool m_projectsPrivacyFlag;
        bool m_filesPrivacyFlag;
        bool m_worksPrivacyFlag;
};

#endif
