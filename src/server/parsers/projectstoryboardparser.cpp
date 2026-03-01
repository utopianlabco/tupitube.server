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
#include "projectstoryboardparser.h"
#include <QTextStream>

ProjectStoryboardParser::ProjectStoryboardParser(const QString &package)
{
    m_sceneIndex = -1;
    m_checksum = 0;
    m_storyboard = "";

    if (m_request.setContent(package)) {
        QDomElement root = m_request.documentElement();
        QDomNode n = root.firstChild();
        while (!n.isNull()) {
               QDomElement e = n.toElement();
               if (e.tagName() == "sceneIndex") {
                   m_sceneIndex = e.text().toInt();
                   m_checksum++;
               } else if (e.tagName() == "storyboard") {
                          QString input = "";
                          QTextStream text(&input);
                          text << n;
                          m_storyboard = input;
                          m_checksum++;
               }
               n = n.nextSibling();
        }
    }
}

ProjectStoryboardParser::~ProjectStoryboardParser()
{
}

bool ProjectStoryboardParser::checksum()
{
    if (m_checksum == 2)
        return true;

    return false;
}

int ProjectStoryboardParser::sceneIndex()
{
    return m_sceneIndex;
}

QString ProjectStoryboardParser::storyboardXml() const
{
    return m_storyboard;
}

QDomDocument ProjectStoryboardParser::request() const
{
    return m_request;
}
