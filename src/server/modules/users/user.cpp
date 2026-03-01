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
#include "user.h"

#include <QList>
#include <QDomDocument>

User::User()
{
}

User::~User()
{
}

void User::setUID(int uid)
{
    m_uid = uid;
}

void User::setName(const QString &name)
{
    m_name = name;
}

void User::setLogin(const QString &login)
{
    m_login = login;
}

void User::setPassword(const QString &password)
{
    m_password = password;
}

void User::setEnabledFlag(bool flag)
{
    m_isEnabled = flag;
}

void User::setCreatorFlag(bool flag)
{
    m_isCreator = flag;
}

void User::setProjectsPrivacyFlag(bool flag)
{
    m_projectsPrivacyFlag = flag;
}

void User::setFilesPrivacyFlag(bool flag)
{
    m_filesPrivacyFlag = flag;
}

void User::setWorksPrivacyFlag(bool flag)
{
    m_worksPrivacyFlag = flag;
}

int User::uid()
{
    return m_uid;
}

QString User::name() const
{
    return m_name;
}

QString User::login() const
{
    return m_login;
}

QString User::password() const
{
    return m_password;
}

bool User::isCreator()
{
    return m_isCreator;
}

bool User::isEnabled()
{
    return m_isEnabled;
}

bool User::projectsPrivacyFlag()
{
    return m_projectsPrivacyFlag;
}

bool User::filesPrivacyFlag()
{
    return m_filesPrivacyFlag;
}

bool User::worksPrivacyFlag()
{
    return m_worksPrivacyFlag;
}

bool User::operator==(const User &user)
{
    return (m_login == user.m_login) && (m_password == user.m_password) && (m_name == user.m_name);
}

