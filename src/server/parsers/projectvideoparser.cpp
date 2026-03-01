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
#include "projectvideoparser.h"

#include <QDebug>

ProjectVideoParser::ProjectVideoParser() : QXmlStreamReader()
{
    m_fps = 0;
}

ProjectVideoParser::ProjectVideoParser(const QString &xml) : QXmlStreamReader(xml)
{
    m_fps = 0;
}

ProjectVideoParser::~ProjectVideoParser()
{
}

bool ProjectVideoParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "video") {
                m_sceneList = attributes().value("scenes").toString();
                m_fps = attributes().value("fps").toInt();
            } else if (tag == "title") {
                m_title = cleanString(readElementText().simplified());
            } else if (tag == "topics") {
                m_topics = cleanString(readElementText().simplified());
            } else if (tag == "description") {
                m_description = cleanString(readElementText().simplified());
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[ProjectVideoParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool ProjectVideoParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

QString ProjectVideoParser::title() const
{
    return m_title;
}

QString ProjectVideoParser::topics() const
{
    return m_topics;
}

QString ProjectVideoParser::description() const
{
    return m_description;
}

QList<int> ProjectVideoParser::scenes() const
{
    QStringList list = m_sceneList.split(",");
    QList<int> result;

    for (int i = 0; i < list.size(); ++i)
         result.append(list.at(i).toInt());

    return result;
}

int ProjectVideoParser::fps()
{
    return m_fps;
}

QString ProjectVideoParser::cleanString(QString input) const
{
    input.replace(",", "\\,");
    input.replace("'", "\"");

    return input;
}
