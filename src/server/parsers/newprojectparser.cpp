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
#include "newprojectparser.h"

#include <QDebug>

NewProjectParser::NewProjectParser() : QXmlStreamReader()
{
    m_fps = 0;
}

NewProjectParser::NewProjectParser(const QString &xml) : QXmlStreamReader(xml)
{
    m_fps = 0;
}

NewProjectParser::~NewProjectParser()
{
}

bool NewProjectParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "author") {
                m_author = readElementText().simplified();
            } else if (tag == "name") {
                m_name = readElementText().simplified();
            } else if (tag == "description") {
                m_description = readElementText().simplified();
            } else if (tag == "bgcolor") {
                m_bgcolor = QColor(readElementText().simplified());
            } else if (tag == "dimension") {
                QString text = readElementText().simplified();
                QStringList list = text.split(",");
                int x = list.at(0).toInt();
                int y = list.at(1).toInt();
                m_dimension = QSize(x, y);
            } else if (tag == "fps") {
                m_fps = readElementText().toInt();
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[NewProjectParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool NewProjectParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

QString NewProjectParser::author() const
{
    return m_author;
}

QString NewProjectParser::name() const
{
    return m_name;
}

QString NewProjectParser::description() const
{
    return m_description;
}

QColor NewProjectParser::bgColor() const
{
    return m_bgcolor;
}

QSize NewProjectParser::dimension() const
{
    return m_dimension;
}

int NewProjectParser::fps() const
{
    return m_fps;
}
