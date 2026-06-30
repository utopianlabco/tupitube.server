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
#include "databasehandler.h"
#include "chatparser.h" // <--- FIX 1: Include the parser that correctly reads the "text" attribute

//base
#include "packagebase.h"
#include "settings.h"
#include "logger.h"
//server/core
#include "server.h"
#include "connection.h"
//server/students
#include "../students/student.h"

#include "global.h"

#include <QDomDocument>

CommunicationManager::CommunicationManager() : Observer()
{
    m_dbHandler = new DatabaseHandler();
}

CommunicationManager::~CommunicationManager()
{
    delete m_dbHandler;
}

void CommunicationManager::handlePackage(PackageBase *const pkg)
{
    #ifdef TUP_DEBUG
        qDebug() << "[CommunicationManager::handlePackage()]";
    #endif

    Connection *cnn = pkg->source();
    Student *student = cnn->student();

    // Retrieve the project filename and convert it to the numeric DB ID
    QString projectFilename = cnn->data(Info::ProjectID).toString();
    int projectID = m_dbHandler->getProjectIdFromFilename(projectFilename);

    if (pkg->root() == "communication_chat") {

        ChatParser parser(pkg->xml());
        QString message = parser.parse() ? parser.message() : "";

        // KEEP: This writes to the actual server log file (tupitube.server.log)
        Logger::self()->info(QObject::tr("Chat from %1: %2").arg(student->login(), message));

        QDomDocument doc;
        doc.setContent(pkg->xml());
        QDomElement element = doc.firstChild().firstChildElement("message");
        element.setAttribute("from", student->login());

        m_dbHandler->saveChatMessage(projectID, student->uid(), student->login(), message, "chat");

        cnn->sendToAll(doc);
        pkg->accept();

    } else if (pkg->root() == "communication_notice") {

        ChatParser parser(pkg->xml());
        QString message = parser.parse() ? parser.message() : "";

        Logger::self()->info(QObject::tr("Notice from %1: %2").arg(student->login(), message));

        QDomDocument doc;
        doc.setContent(pkg->xml());
        QDomElement element = doc.firstChild().firstChildElement("message");
        element.setAttribute("from", student->login());

        m_dbHandler->saveChatMessage(projectID, student->uid(), student->login(), message, "notice");

        cnn->sendToAll(doc);
        pkg->accept();

    } else if (pkg->root() == "communication_wall") {

        ChatParser parser(pkg->xml());
        QString message = parser.parse() ? parser.message() : "";

        Logger::self()->info(QObject::tr("Wall post from %1: %2").arg(student->login(), message));

        QDomDocument doc;
        doc.setContent(pkg->xml());
        QDomElement element = doc.firstChild().firstChildElement("message");
        element.setAttribute("from", student->login());

        m_dbHandler->saveChatMessage(projectID, student->uid(), student->login(), message, "wall");

        cnn->sendToAll(doc);
        pkg->accept();
    }
}
