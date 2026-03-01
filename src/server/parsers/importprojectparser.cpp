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
#include "importprojectparser.h"

#include <QDebug>

ImportProjectParser::ImportProjectParser() : QXmlStreamReader()
{
}

ImportProjectParser::ImportProjectParser(const QString &xml) : QXmlStreamReader(xml)
{
}

ImportProjectParser::~ImportProjectParser()
{
}

bool ImportProjectParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "data") {
                m_path = attributes().value("file").toString();
                QString text = readElementText().simplified();
                m_data = QByteArray::fromBase64(text.toLocal8Bit());
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[ImportProjectParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool ImportProjectParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

QByteArray ImportProjectParser::data() const
{
    return m_data;
}

QString ImportProjectParser::path() const
{
    return m_path;
}
