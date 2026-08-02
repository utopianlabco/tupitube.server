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

#include "commandresult.h"

CommandResult::CommandResult(
    const QString &commandId,
    Status status,
    const QString &errorCode,
    const QString &message)
    : QDomDocument(),
      m_commandId(commandId),
      m_status(status),
      m_errorCode(errorCode),
      m_message(message)
{
    QDomElement root = createElement(QStringLiteral("command_result"));
    root.setAttribute(QStringLiteral("version"), QStringLiteral("1"));
    root.setAttribute(QStringLiteral("command_id"), m_commandId);
    root.setAttribute(QStringLiteral("status"), statusToString(m_status));

    if (!m_errorCode.isEmpty()) {
        root.setAttribute(
            QStringLiteral("error_code"),
            m_errorCode);
    }

    appendChild(root);

    if (!m_message.isEmpty()) {
        QDomElement messageElement =
            createElement(QStringLiteral("message"));

        messageElement.appendChild(
            createTextNode(m_message));

        root.appendChild(messageElement);
    }
}

CommandResult::~CommandResult()
{
}

QString CommandResult::commandId() const
{
    return m_commandId;
}

CommandResult::Status CommandResult::status() const
{
    return m_status;
}

QString CommandResult::errorCode() const
{
    return m_errorCode;
}

QString CommandResult::message() const
{
    return m_message;
}

QString CommandResult::statusToString(Status status)
{
    switch (status) {
        case Committed:
            return QStringLiteral("committed");

        case Rejected:
            return QStringLiteral("rejected");

        case Failed:
            return QStringLiteral("failed");
    }

    return QStringLiteral("failed");
}
