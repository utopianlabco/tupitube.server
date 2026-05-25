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
#include "logger.h"
#include "tconfig.h"

#include <QFile>
#include <QDateTime>
#include <QDebug>


Logger *Logger::s_self = nullptr;
bool Logger::s_destroyed = false;

Logger::Logger()
{
    TCONFIG->beginGroup("General");
    QString path = TCONFIG->value("LogPath").toString() + "/" + "tupitube.server.log";

    #ifdef TUP_DEBUG
        qDebug() << "[Logger::Logger()] Starting log file at ->" << path;
    #endif

    m_file.setFileName(path);
}

Logger::~Logger()
{
    if (s_destroyed) return;
#ifdef TUP_DEBUG
    qDebug() << "[Logger::~Logger()] Destructor called. m_file.isOpen():" << m_file.isOpen() << ", fileName:" << m_file.fileName();
#endif
    if (m_file.isOpen()) {
        m_file.close();
#ifdef TUP_DEBUG
        qDebug() << "[Logger::~Logger()] File closed in destructor.";
#endif
    }
    m_file.setFileName("");
    s_destroyed = true;
}

Logger *Logger::self()
{
    if (s_destroyed) {
        return nullptr;
    }
    if (!s_self)
        s_self = new Logger;
    return s_self;
}

void Logger::setLogFile(const QString &logfile)
{
    m_file.setFileName(logfile);
}

QString Logger::logFile() const
{
    return m_file.fileName();
}

void Logger::warn(const QString &log)
{
    Logger *logger = Logger::self();
    if (!logger) return;
    logger->write("[" + QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] [WARN] " + log + "\n").toLocal8Bit());
}

void Logger::error(const QString &log)
{
    Logger *logger = Logger::self();
    if (!logger) return;
    logger->write("[" + QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] [ERROR] " + log + "\n").toLocal8Bit());
}

void Logger::info(const QString &log)
{
    Logger *logger = Logger::self();
    if (!logger) return;
    logger->write("[" + QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] [INFO] " + log + "\n").toLocal8Bit());
}

void Logger::fatal(const QString &log)
{
    Logger *logger = Logger::self();
    if (!logger) return;
    logger->write("[" + QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] [FATAL] " + log + "\n").toLocal8Bit());
}

void Logger::write(const QByteArray &msg)
{
    if (!m_file.fileName().isEmpty()) {
        if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_file.write(msg.data(), msg.size());
            m_file.close();
        } else {
#ifdef TUP_DEBUG
            qDebug() << "[Logger::write] Failed to open log file:" << m_file.fileName() << ", error:" << m_file.errorString();
#endif
        }
    } else {
#ifdef TUP_DEBUG
        qDebug() << "[Logger::write] m_file.fileName() is empty, skipping write.";
#endif
    }
}

void Logger::reset()
{
    if (s_destroyed || !s_self) return;
    if (s_self->m_file.isOpen()) {
        s_self->m_file.close();
    }
    s_self->m_file.setFileName("");
    Logger *toDelete = s_self;
    s_self = nullptr;
    s_destroyed = true;
    delete toDelete;
}
