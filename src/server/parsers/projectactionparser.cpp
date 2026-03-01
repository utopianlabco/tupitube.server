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
#include "projectactionparser.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDebug>

ProjectActionParser::ProjectActionParser() : QXmlStreamReader()
{
    m_fps = 0;
    m_type = 0;
}

ProjectActionParser::ProjectActionParser(const QString &xml) : QXmlStreamReader(xml)
{
    m_fps = 0;
    m_type = 0;
}

ProjectActionParser::~ProjectActionParser()
{
}

bool ProjectActionParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "user") {
                m_type = attributes().value("type").toInt();
                QString text = readElementText().simplified();
                m_users.insert(m_type, text);
            } else if (tag == "name") {
                m_name = readElementText().simplified();
            } else if (tag == "author") {
                m_author = readElementText().simplified();
            } else if (tag == "description") {
                m_description = readElementText().simplified();
            } else if (tag == "bgcolor") {
                QString text = readElementText().simplified();
                m_bgcolor = QColor(text);
            } else if (tag == "dimension") {
                QString text = readElementText().simplified();
                QStringList list = text.split(",");
                int x = list.at(0).toInt();
                int y = list.at(1).toInt();
                QSize size(x, y);
                m_dimension = size;
            } else if (tag == "fps") {
                QString text = readElementText().simplified();
                m_fps = text.toInt();
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[ProjectActionParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool ProjectActionParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

QString ProjectActionParser::name()
{
    return m_name;
}

QString ProjectActionParser::author()
{
    return m_author;
}

QString ProjectActionParser::description()
{
    return m_description;
}

QColor ProjectActionParser::bgcolor() const
{
    return m_bgcolor;
}

QSize ProjectActionParser::dimension() const
{
    return m_dimension;
}

int ProjectActionParser::fps() const
{
    return m_fps;
}

QMultiHash<int, QString> ProjectActionParser::users()
{
    return m_users;
}

QString ProjectActionParser::package(QString package, const QString &date)
{
    QDomDocument doc;

    if (doc.setContent(package.trimmed())) {
        QDomElement root = doc.firstChildElement("project_add");
        QDomElement dateE = doc.createElement("date"); 
        dateE.appendChild(doc.createTextNode(date));
        root.appendChild(dateE);

        return doc.toString();
    }

    return "";
}
