/***************************************************************************
 *   Project TupiTube Server                                               *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License: GNU General Public License version 2 or later.                *
 ***************************************************************************/

#ifndef COMMANDRESULTREGISTRY_H
#define COMMANDRESULTREGISTRY_H

#include <QHash>
#include <QQueue>
#include <QString>

class CommandResultRegistry
{
public:
    enum Status
    {
        Committed = 0,
        Rejected,
        Failed
    };

    struct StoredResult
    {
        Status status = Failed;
        QString errorCode;
        QString message;
    };

    explicit CommandResultRegistry(int maximumResults = 10000);

    bool contains(const QString &commandId) const;
    StoredResult result(const QString &commandId) const;

    void store(
        const QString &commandId,
        Status status,
        const QString &errorCode = QString(),
        const QString &message = QString());

    void clear();
    int count() const;
    int maximumResults() const;
    void setMaximumResults(int maximumResults);

private:
    void trimToLimit();

private:
    QHash<QString, StoredResult> m_results;
    QQueue<QString> m_insertionOrder;
    int m_maximumResults;
};

#endif
