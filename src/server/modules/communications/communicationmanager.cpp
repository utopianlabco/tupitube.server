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
#include "communicationmanager.h"
//base
#include "packagebase.h"
#include "settings.h"
#include "logger.h"
//server/core
#include "server.h"
#include "connection.h"
//server/users
#include "user.h"

#include <QDomDocument>
#include <QDebug>
    
CommunicationManager::CommunicationManager() : Observer()
{
}

CommunicationManager::~CommunicationManager()
{
}

void CommunicationManager::handlePackage(PackageBase *const pkg)
{
    #ifdef TUP_DEBUG
        qDebug() << "[CommunicationManager::handlePackage()]";
    #endif

    Connection *cnn = pkg->source();
    
    if (pkg->root() == "communication_chat") {

        QDomDocument doc;
        doc.setContent(pkg->xml());
        
        QDomElement element = doc.firstChild().firstChildElement("message");
        element.setAttribute("from", cnn->user()->login());
        cnn->sendToAll(doc);
        pkg->accept();

    } else if (pkg->root() == "communication_notice") {

        QDomDocument doc;
        doc.setContent(pkg->xml());
        
        QDomElement element = doc.firstChild().firstChildElement("message");
        element.setAttribute("from", cnn->user()->login());
        cnn->sendToAll(doc); //TODO: enviar a todos los clientes del proyecto
        pkg->accept();

    } else if (pkg->root() == "communication_wall") {

        QDomDocument doc;
        doc.setContent(pkg->xml());
        QDomElement element = doc.firstChild().firstChildElement("message");
        element.setAttribute("from", cnn->user()->login());
        cnn->sendToAll(doc);
        pkg->accept();

    }
}
