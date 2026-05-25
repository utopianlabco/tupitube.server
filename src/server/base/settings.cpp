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
#include "settings.h"

Settings *Settings::s_settings = nullptr;
bool Settings::s_destroyed = false;

Settings::Settings()
    : m_repositoryPath(), m_backupPath()
{
}

Settings::~Settings()
{
}

Settings *Settings::self()
{
    if (s_destroyed) return nullptr;
    if (!s_settings)
        s_settings = new Settings();
    return s_settings;
}

void Settings::reset()
{
    if (s_destroyed || !s_settings) return;
    Settings *toDelete = s_settings;
    s_settings = nullptr;
    s_destroyed = true;
    delete toDelete;
}

void Settings::setRepositoryPath(const QString &repository)
{
    if (repository.isEmpty() || repository.endsWith("/"))
        m_repositoryPath = repository;
    else
        m_repositoryPath = repository + "/";
}

QString Settings::repositoryPath() const
{
    return m_repositoryPath;
}

void Settings::setBackupPath(const QString &backupPath)
{
    if (backupPath.isEmpty() || backupPath.endsWith("/"))
        m_backupPath = backupPath;
    else
        m_backupPath = backupPath + "/";
}

QString Settings::backupPath() const
{
    return m_backupPath;
}
