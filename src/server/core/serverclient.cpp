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

#include "serverclient.h"
#include "connection.h"

#include <QDataStream>
#include <QStringList>

Client::Client(Connection *connection) : SocketBase(), m_connection(connection)
{
}

Client::~Client()
{
}

void Client::readed(const QString &package)
{
    m_connection->appendTextReaded(package);
}

void Client::send(const QString &text)
{
    SocketBase::send(text);
}

void Client::send(const QDomDocument &doc)
{
    SocketBase::send(doc);
}

void Client::sendFile(const QString &path)
{
    SocketBase::sendFile(path);
}

void Client::flush()
{
    SocketBase::flush();
}

void Client::disconnectFromHost()
{
    SocketBase::disconnectFromHost();
}

void Client::close()
{
    SocketBase::close();
}
