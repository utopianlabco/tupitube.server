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
#include "netproject.h"
#include "filemanager.h"
#include "tupscene.h"
#include "tuplayer.h"
#include "tapplicationproperties.h"
#include "talgorithm.h"

#include <QFileInfo>
#include <QTime>
#include <QDate>
#include <QDir>
#include <QDebug>

NetProject::NetProject(QObject *parent) : TupProject(parent)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NetProject::NetProject()]";
    #endif

    TupScene *scene = createScene("Scene 1", 0, false);
    TupLayer *layer = scene->createLayer("Layer 1", 0, false);
    layer->createFrame("Frame 1", 0, false);
    
    m_saveTimer = new QTimer(this);
    m_saveTimer->setInterval(30000);
    m_saveTimer->start();
    m_timerId = startTimer(300000);

    m_file = "";
}

NetProject::~NetProject()
{
    delete m_saveTimer;
    killTimer(m_timerId);
}

void NetProject::setProjectParams(int userID)
{
    m_owner = userID;
    QList<QString> date = currentTime();
    QString year  = date.at(0);
    QString month = date.at(1);
    QString day   = date.at(2);
    QString hour  = date.at(3);
    QString min   = date.at(4);
    QString sec   = date.at(5);

    m_date = year + "-" + month + "-" + day + " " + hour + ":" + min + ":" + sec;

    m_file = TAlgorithm::randomString(20);
    // m_file = QString::number(m_owner) + "-" + year + month + day + hour + min + sec;

    QString cacheDir = CACHE_DIR;
    if (cacheDir.endsWith("/"))
        cacheDir.chop(1);
    setDataDir(cacheDir + "/" + QString::number(m_owner) + "/" + m_file);
}

QList<QString> NetProject::currentTime()
{
    QList<QString> dateList;

    QDate date = QDate::currentDate();

    QString year = QString::number(date.year());

    dateList.append(year);

    int mon = date.month();
    QString month = QString::number(mon);
    if (mon < 10)
        month = "0" + month;

    dateList.append(month);

    int d = date.day();
    QString day = QString::number(d);
    if (d < 10)
        day = "0" + day;

    dateList.append(day);

    QTime time = QTime::currentTime();
    int h = time.hour();
    QString hour = QString::number(h);
    if (h < 10)
        hour = "0" + hour;

    dateList.append(hour);

    int m = time.minute();
    QString min = QString::number(m);
    if (m < 10)
        min = "0" + min;

    dateList.append(min);

    int s = time.second();
    QString sec = QString::number(s);
    if (s < 10)
        sec = "0" + sec;

    dateList.append(sec);

    return dateList;
} 

QString NetProject::fileCode()
{
    /*
    QString code = "";
    QList<QString> date = currentTime();
    for (int i=0; i < date.size(); i++)
         code += date.at(i);

    m_file = TAlgorithm::randomString(20);
    */

    return TAlgorithm::randomString(20);
}

void NetProject::resetTimer()
{
    m_saveTimer->stop();
    m_saveTimer->start();
}

bool NetProject::save(bool quiet)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NetProject::save()]";
    #endif

    if (m_file.length() == 0) {
        // SQA: Check when this condition is true / it never should happen
        qWarning() << "[NetProject::save()] - Error: File name now is NULL!";

        m_saveTimer->stop();
        return false;
    }

    FileManager *manager = new FileManager;
    bool ok = manager->save(m_file, this, m_owner);
    
    if (ok) {
        #ifdef TUP_DEBUG
               qWarning() << "[NetProject::save()] - Project saved successfully";
        #endif
        if (!quiet)
            emit requestSendMessage(380, QObject::tr("Project saved successfully"), Notification::Info);
    } else {
        if (!quiet)
            emit requestSendMessage(381, QObject::tr("Error saving project %1").arg(getName()), Notification::Error);
    }

    delete manager;

    return ok;
}

void NetProject::timerEvent(QTimerEvent *)
{
    save();
}

bool NetProject::addUser(int userID)
{
    if (!m_users.contains(userID)) {
        m_users.append(userID);
        return true;
    }

    return false;
}

void NetProject::setUsers(const QList<int> &users)
{
    m_users = users;
}

void NetProject::setFilename(const QString &file)
{
    m_file = file;
}

QString NetProject::filename() const
{
    return m_file;
}

bool NetProject::isOwner(int userID)
{
    return userID == m_owner;
}

QString NetProject::date() const
{
    return m_date;
}

void NetProject::setOwner(int owner)
{
    m_owner = owner;
}

int NetProject::owner() const
{
    return m_owner;
}
