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
#include "student.h"

#include <QList>
#include <QDomDocument>

Student::Student()
{
}

Student::~Student()
{
}

void Student::setUID(int uid)
{
    m_uid = uid;
}

void Student::setName(const QString &name)
{
    m_name = name;
}

void Student::setLogin(const QString &login)
{
    m_login = login;
}

void Student::setPassword(const QString &password)
{
    m_password = password;
}

void Student::setEnabledFlag(bool flag)
{
    m_isEnabled = flag;
}

void Student::setCreatorFlag(bool flag)
{
    m_isCreator = flag;
}

void Student::setProjectsPrivacyFlag(bool flag)
{
    m_projectsPrivacyFlag = flag;
}

void Student::setFilesPrivacyFlag(bool flag)
{
    m_filesPrivacyFlag = flag;
}

void Student::setWorksPrivacyFlag(bool flag)
{
    m_worksPrivacyFlag = flag;
}

int Student::uid()
{
    return m_uid;
}

QString Student::name() const
{
    return m_name;
}

QString Student::login() const
{
    return m_login;
}

QString Student::password() const
{
    return m_password;
}

bool Student::isCreator()
{
    return m_isCreator;
}

bool Student::isEnabled()
{
    return m_isEnabled;
}

bool Student::projectsPrivacyFlag()
{
    return m_projectsPrivacyFlag;
}

bool Student::filesPrivacyFlag()
{
    return m_filesPrivacyFlag;
}

bool Student::worksPrivacyFlag()
{
    return m_worksPrivacyFlag;
}

bool Student::operator==(const Student &student)
{
    return (m_login == student.m_login) && (m_password == student.m_password) && (m_name == student.m_name);
}

