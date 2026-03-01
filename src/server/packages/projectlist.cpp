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
#include "projectlist.h"

ProjectList::ProjectList() : Package()
{
    QDomElement root = createElement("server_projectlist");
    root.setAttribute("version",  "0");
    appendChild(root);

    QDomElement works = createElement("works");
    root.appendChild(works);

    QDomElement contributions = createElement("contributions");
    root.appendChild(contributions);
}

ProjectList::~ProjectList()
{
}

void ProjectList::addProject(ProjectType type, const QString &filename, const QString &name, 
                             const QString &author, const QString &description, const QString &date)
{
    QDomNode root = firstChild();
    QDomNode node = root.firstChildElement("contributions");

    if (type == Work)
        node = root.firstChildElement("works");
    
    QDomElement project = createElement("project");
    project.setAttribute("filename", filename);
    project.setAttribute("name", name);

    if (type == Contribution)
        project.setAttribute("author", author);

    project.setAttribute("description", description);
    project.setAttribute("date", date);
    
    node.appendChild(project);
}

