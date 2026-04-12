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
#include "project.h"

#include <QFile>
#include <QDebug>

/*
<!-- Answer to valid open request -->
<server_project version="0">
        <students>
            login1,login2,login3,etc
        </students> 
        <data>
            <![CDATA[TUP project encoded with Base64]]>
        </data>
</server_project>
*/

Project::Project(const QString &students, const QString &projectPath): Package()
{
    QDomElement root = createElement("server_project");
    root.setAttribute("version", "0");
    appendChild(root);

    root.appendChild(createElement("students")).appendChild(createTextNode(students));

    QFile file(projectPath);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll().toBase64();
    
    QDomElement projectContent = createElement("data");
    projectContent.appendChild(createCDATASection(data));
    root.appendChild(projectContent);
}

Project::~Project()
{
}

