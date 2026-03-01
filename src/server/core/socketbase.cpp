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
#include "socketbase.h"

#include <QFile>
#include <QDebug>

SocketBase::SocketBase(QObject *parent) : QTcpSocket(parent)
{
    connect(this, SIGNAL(readyRead ()), this, SLOT(readFromServer()));
    connect(this, SIGNAL(connected()), this, SLOT(sendQueue()));
    connect(this, SIGNAL(disconnected()), this, SLOT(clearQueue()));
}

SocketBase::~SocketBase()
{
}

void SocketBase::sendQueue()
{
    while (m_queue.count() > 0) {
        if (state() == QAbstractSocket::ConnectedState)
            send(m_queue.dequeue());
        else 
            break;
    }
}

void SocketBase::clearQueue()
{
    m_queue.clear();
}

void SocketBase::send(const QString &message)
{
    if (state() == QAbstractSocket::ConnectedState) {
        QTextStream stream(this);
        stream.setCodec("UTF-8");
        stream << message.toUtf8().toBase64() << "%%" << Qt::endl;
    } else {
        m_queue.enqueue(message);
    }
}

void SocketBase::send(const QDomDocument &doc)
{
    send(doc.toString(0));
}

void SocketBase::sendFile(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        qint64 fileSize = file.size();
        int blockSize = 128;
        int blockCounter = 0;
        int index = 0;

        #ifdef TUP_DEBUG
            qDebug() << "[SocketBase::sendFile()] - Transmiting media file -> " << path;
            qDebug() << "[SocketBase::sendFile()] - media size: " << (int) fileSize;
        #endif

        while (index < fileSize) {
            QByteArray segment = data.mid(index, blockSize);
            qint64 result = write(segment);
            blockCounter++;
            if (result == -1) {
                qDebug() << "[SocketBase::sendFile()] - Fatal Error: Couldn't write to socket!";
            } else {
                if ((index + blockSize) == fileSize) {
                    break;
                } else {
                    index += blockSize;
                    if ((index + blockSize) > fileSize)
                        blockSize = fileSize - index;
                }
            }
        }

        #ifdef TUP_DEBUG
            qDebug() << "[SocketBase::sendFile()] - blockCounter: " << blockCounter;
        #endif
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[SocketBase::sendFile()] - Fatal Error: Couldn't read file -> " << path; 
        #endif
    }

    #ifdef TUP_DEBUG
        qDebug() << "[SocketBase::sendFile()] - Done!";
    #endif

    file.close();
}

void SocketBase::readFromServer()
{
    QString readed = "";

    while (this->canReadLine()) {
           readed += QString::fromUtf8(this->readLine());
           if (readed.endsWith("%%\n"))
               break;
    }
    
    if (!readed.isEmpty()) {
        readed.remove(readed.lastIndexOf("%%"), 2);
        readed = QString::fromUtf8(QByteArray::fromBase64(readed.toUtf8()));
        
        this->readed(readed);
    }
    
    if (this->canReadLine()) 
        readFromServer();
}

