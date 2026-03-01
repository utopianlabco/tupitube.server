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
#include "listparser.h"

#include <QDebug>

ListParser::ListParser(): QXmlStreamReader()
{
    m_caseSensitive = false;
    m_regexp = false;
    m_type = 0;
}

ListParser::ListParser(const QString &xml): QXmlStreamReader(xml)
{
    m_caseSensitive = false;
    m_regexp = false;
    m_type = 0;
}

ListParser::~ListParser()
{
}

bool ListParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "options") {
                m_pattern = attributes().value("pattern").toString();
                m_type = attributes().value("type").toInt();
            } else if (tag == "caseSensitive") {
                m_caseSensitive = bool(attributes().value("enabled").toInt());
            } else if (tag == "regexp") {
                m_regexp = bool(attributes().value("enabled").toInt());
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[ListParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool ListParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

bool ListParser::isRegexp()
{
    return m_regexp;
}

bool ListParser::isCaseSensitive()
{
    return m_caseSensitive;
}

QString ListParser::pattern()
{
    return m_pattern;
}

int ListParser::type()
{
    return m_type;
}
