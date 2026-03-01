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
#ifndef PROJECTACTIONPARSER_H
#define PROJECTACTIONPARSER_H

#include <QXmlStreamReader>
#include <QString>
#include <QColor>
#include <QSize>
#include <QMultiHash>

class ProjectActionParser : public QXmlStreamReader
{
    public:
        ProjectActionParser();
        ProjectActionParser(const QString &xml);
        ~ProjectActionParser();
        
        bool parse();
        bool parse(const QString &xml);
        
        QString name();
        QString author();
        QString description();
        QColor bgcolor() const;
        QSize dimension() const;
        int fps() const;
        QMultiHash<int, QString> users();

        QString package(QString package, const QString &date);
        
    private:
        QString m_name;
        QString m_author;
        QString m_description;
        QColor m_bgcolor;
        QSize m_dimension;
        int m_fps;
        QMultiHash<int, QString> m_users;
        int m_type;
};

#endif
