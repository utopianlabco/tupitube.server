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
#ifndef NETPROJECT_H
#define NETPROJECT_H

#include "tupproject.h"
#include "notification.h"
#include "user.h"

#include <QTimer>
#include <QList>

class NetProject : public TupProject
{
    Q_OBJECT
    
    public:

        NetProject(QObject *parent = nullptr);
        ~NetProject();

        void setProjectParams(int userID);

        void resetTimer();
        bool addUser(int userID);
        void setUsers(const QList<int> &users);
        void setFilename(const QString &file);
        QString filename() const;
        
        bool isOwner(int userID);
        QString date() const;
        void setOwner(int owner);
        int owner() const;

        QString fileCode();

    private:
        QList<QString> currentTime();
        QTimer *m_saveTimer;
        QString m_date;
        QString m_file;
        int m_owner;
        QList<int> m_users;
        int m_timerId;
    
    public slots:
        bool save(bool quiet = false);
        
    signals:
        void requestSendMessage(int code, const QString &message, Notification::Level level);
        
    protected:
        void timerEvent(QTimerEvent * event);
};

#endif
