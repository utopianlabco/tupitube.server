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
#include "projectimageparser.h"

#include <QDebug>

ProjectImageParser::ProjectImageParser() : QXmlStreamReader()
{
    m_scene = 0;
    m_frame = 0;
}

ProjectImageParser::ProjectImageParser(const QString &xml) : QXmlStreamReader(xml)
{
    m_scene = 0;
    m_frame = 0;
}

ProjectImageParser::~ProjectImageParser()
{
}

bool ProjectImageParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "image") {
                m_scene = attributes().value("scene").toInt();
                m_frame = attributes().value("frame").toInt();
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
            qDebug() << "[ProjectImageParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool ProjectImageParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

QString ProjectImageParser::title() const
{
    return m_title;
}

QString ProjectImageParser::topics() const
{
    return m_topics;
}

QString ProjectImageParser::description() const
{
    return m_description;
}

int ProjectImageParser::scene() const
{
    return m_scene;
}

int ProjectImageParser::frame() const
{
    return m_frame;
}

QString ProjectImageParser::cleanString(QString input) const
{
    input.replace(",", "\\,");
    input.replace("'", "\"");

    return input;
}

