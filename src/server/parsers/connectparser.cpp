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
#include "connectparser.h"

#include <QDebug>

ConnectParser::ConnectParser() : QXmlStreamReader()
{
    m_client = 0;
    m_flag1 = false;
    m_flag2 = false;
    m_flag3 = false;
}

ConnectParser::ConnectParser(const QString &xml) : QXmlStreamReader(xml)
{
    m_client = 0;
    m_flag1 = false;
    m_flag2 = false;
    m_flag3 = false;
}

ConnectParser::~ConnectParser()
{
}

bool ConnectParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = QXmlStreamReader::name().toString();
            if (tag == "username") {
                m_username = readElementText().simplified();
            } else if (tag == "password") {
                QString text = readElementText().simplified();
                if (!m_flag1) {
                    m_password1 = text;
                    m_flag1 = true;
                } else if (!m_flag2) {
                    m_password2 = text;
                    m_flag2 = true;
                } else if (!m_flag3) {
                    m_password3 = text;
                    m_flag3 = true;
                }
            } else if (tag == "client") {
                m_client = attributes().value("type").toInt();
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[ConnectParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

bool ConnectParser::parse(const QString &xml)
{
    clear();
    addData(xml);
    return parse();
}

QString ConnectParser::username() const
{
    return m_username;
}

QString ConnectParser::password() const
{
    // Handle single password mode (MD5 hash from local servers)
    if ((m_password1.length() > 0) && (m_password2.length() == 0) && (m_password3.length() == 0)) {
        // Single password mode - return the password prefixed with "md5:" for verification
        return "md5:" + m_password1;
    }

    // Handle multi-password obfuscation mode (for tupitu.be)
    if ((m_password1.length() > 0) && (m_password2.length() > 0) && (m_password3.length() > 0)) {
         // QByteArray array = m_username.toAscii();
         QByteArray array = m_username.toUtf8(); 
         char letter = array.at(0);
         int ascii = static_cast<int>(letter);
         int mod = ascii%3;
         QString passwd = "";

         switch (mod) {
                 case 0:
                 {
                      QString p = m_password1;
                      p.remove(30, 1);
                      p.remove(22, 50);
                      passwd = p;
                 }
                 break;
                 case 1:
                 {
                      QString p = m_password2;
                      p.remove(30, 1);
                      p.remove(22, 50);
                      passwd = p;
                 }
                 break;
                 case 2:
                 {
                      QString p = m_password3;
                      p.remove(30, 1);
                      p.remove(22, 50);
                      passwd = p;
                 }
                 break;
         }

         return passwd;
    }

    return "";
}

int ConnectParser::clientType() const
{
    return m_client;
}
