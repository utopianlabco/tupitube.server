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
#include "notification.h"

/*
INFO CODES
- Images
  100 : Image posted successfully
  101 : Video posted successfully
  102 : Storyboard posted successfully
  
ERROR CODES

- Projects
  300 : Cannot create project %1
  301 : Cannot update project %1
  302 : Cannot remove project %1

  320 : Project %1 already exists.<br/>Please, try another name
  321 : Project %1 doesn't exist
  322 : Project %1 is busy
  323 : Error while loading project %1
  324 : Error while importing project %1 

  340 : Cannot handle project request

  360 : Insufficient permissions

  380 : Project saved successfully 
  381 : Error saving project %1 
  382 : Error while posting image %1
  383 : Error while posting video %1
  384 : Error while posting storyboard %1

- Users
  400 : Invalid login or password
  401 : Error adding user %1
  402 : User doesn't exist 
  403 : Invalid/Corrupted package

  460 : Insufficient permissions

- Bans
  560 : Insufficient permissions

- Backups
  660 : Insufficient permissions
*/

Notification::Notification(int code, const QString &message, Level level) : QDomDocument()
{
    QDomElement root = createElement("communication_notification");
    root.setAttribute("version", "0");
    appendChild(root);
    
    m_message = createElement("message");
    m_message.setAttribute("code", code); 
    root.appendChild(m_message);
    m_message.appendChild(createTextNode(message));
    m_message.setAttribute("level", level);
}

Notification::~Notification()
{
}

void Notification::setMessage(const QString &message)
{
    m_text.setData(message);
}

void Notification::setLevel(int level)
{
    m_message.setAttribute("level", level);
}
