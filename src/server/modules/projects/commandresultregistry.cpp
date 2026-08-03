/***************************************************************************
 *   Project TupiTube Server                                               *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License: GNU General Public License version 2 or later.                *
 ***************************************************************************/

#include "commandresultregistry.h"

CommandResultRegistry::CommandResultRegistry(int maximumResults)
    : m_maximumResults(maximumResults > 0 ? maximumResults : 1)
{
}

bool CommandResultRegistry::contains(const QString &commandId) const
{
    return !commandId.isEmpty() && m_results.contains(commandId);
}

CommandResultRegistry::StoredResult CommandResultRegistry::result(
    const QString &commandId) const
{
    return m_results.value(commandId);
}

void CommandResultRegistry::store(
    const QString &commandId,
    Status status,
    const QString &errorCode,
    const QString &message)
{
    if (commandId.isEmpty())
        return;

    StoredResult stored;
    stored.status = status;
    stored.errorCode = errorCode;
    stored.message = message;

    if (!m_results.contains(commandId))
        m_insertionOrder.enqueue(commandId);

    m_results.insert(commandId, stored);
    trimToLimit();
}

void CommandResultRegistry::clear()
{
    m_results.clear();
    m_insertionOrder.clear();
}

int CommandResultRegistry::count() const
{
    return m_results.count();
}

int CommandResultRegistry::maximumResults() const
{
    return m_maximumResults;
}

void CommandResultRegistry::setMaximumResults(int maximumResults)
{
    m_maximumResults = maximumResults > 0 ? maximumResults : 1;
    trimToLimit();
}

void CommandResultRegistry::trimToLimit()
{
    while (m_results.count() > m_maximumResults
           && !m_insertionOrder.isEmpty()) {
        const QString oldestCommandId = m_insertionOrder.dequeue();
        m_results.remove(oldestCommandId);
    }
}
