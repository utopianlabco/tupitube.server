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

namespace Packages
{
    
Items::Items()
 : Package()
{
    m_root = createElement("items");
    m_root.setAttribute("version", "0");
    appendChild(m_root);
}

Items::~Items()
{
    
}

void Items::addItem(const QString& item)
{
    m_root.appendChild(createElement("item")).appendChild(createTextNode(item));
}

void Items::addItems(const QStringList& items)
{
    foreach (const QString item, items)
             addItem(item);
}

void Items::setItems( const QStringList& items)
{
    removeChild (m_root);
    m_root = createElement("items");
    m_root.setAttribute("version",  "0");
    appendChild(m_root);
    
    addItems(items);
}

}
