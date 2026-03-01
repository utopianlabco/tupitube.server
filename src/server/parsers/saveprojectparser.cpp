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
#include "saveprojectparser.h"

#include <QDebug>

SaveProjectParser::SaveProjectParser() : QXmlStreamReader()
{
    m_exit = 0;
}

SaveProjectParser::SaveProjectParser(const QString &xml) : QXmlStreamReader(xml)
{
    m_exit = 0;
}

SaveProjectParser::~SaveProjectParser()
{
}

bool SaveProjectParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "exit") {
                m_exit = attributes().value("exit").toInt();
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[SaveProjectParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool SaveProjectParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

int SaveProjectParser::exit()
{
    return m_exit;
}
