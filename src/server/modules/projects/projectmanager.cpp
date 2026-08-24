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
#include "projectmanager.h"

#include "connection.h"
#include "tupproject.h"
#include "tuplayer.h"
#include "tupframe.h"
#include "tupscene.h"
#include "tupbackground.h"
#include "tupgraphicobject.h"
#include "tupitemconverter.h"
#include "tupserializer.h"
#include "tupsvg2qt.h"
#include "genericexportplugin.h"
#include "tupexportinterface.h"
#include "tuprequestparser.h"
#include "tuprequestbuilder.h"
#include "filemanager.h"
#include "tupcommandexecutor.h"
#include "tupprojectcommand.h"
#include "server.h"
#include "global.h"
#include "project.h"
#include "../students/student.h"
#include "projectactionparser.h"
#include "newprojectparser.h"
#include "openprojectparser.h"
#include "saveprojectparser.h"
#include "importprojectparser.h"
#include "projectimageparser.h"
#include "talgorithm.h"
#include "projectvideoparser.h"
#include "projectstoryboardparser.h"
#include "projectstoryboardpostparser.h"
#include "listparser.h"
#include "listprojectsparser.h"
#include "packagebase.h"
#include "notice.h"
#include "notification.h"
#include "commandresult.h"
#include "tapplicationproperties.h"
#include "logger.h"

#include <QDir>
#include <QDomDocument>
#include <QHash>
#include <QColor>
#include <QDebug>
#include <QCryptographicHash>
#include <QUuid>
#include <QSet>
#include <QPainterPath>
#include <QStringList>

namespace
{
    TupFrame *conversionFrameForResponse(TupItemResponse *response, NetProject *project)
    {
        if (!response || !project)
            return nullptr;

        TupScene *scene = project->sceneAt(response->getSceneIndex());
        if (!scene)
            return nullptr;

        if (response->spaceMode() == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(response->getLayerIndex());
            return layer ? layer->frameAt(response->getFrameIndex()) : nullptr;
        }

        TupBackground *background = scene->sceneBackground();
        if (!background)
            return nullptr;

        if (response->spaceMode() == TupProject::VECTOR_STATIC_BG_MODE)
            return background->vectorStaticFrame();
        if (response->spaceMode() == TupProject::VECTOR_FG_MODE)
            return background->vectorForegroundFrame();
        if (response->spaceMode() == TupProject::VECTOR_DYNAMIC_BG_MODE)
            return background->vectorDynamicFrame();

        return nullptr;
    }

    bool extractConversionSourceSnapshot(
        const QString &payload, QString *snapshot)
    {
        if (!snapshot)
            return false;

        QDomDocument document;
        if (!document.setContent(payload))
            return false;

        const QDomNodeList nodes = document.elementsByTagName(
            QStringLiteral("conversion_source"));
        if (nodes.count() == 0)
            return false;

        const QDomElement element = nodes.at(0).toElement();
        if (element.attribute(QStringLiteral("encoding"))
                != QStringLiteral("base64")) {
            return false;
        }

        const QByteArray decoded = QByteArray::fromBase64(
            element.text().trimmed().toLatin1());
        if (decoded.isEmpty())
            return false;

        *snapshot = QString::fromUtf8(decoded);
        return !snapshot->trimmed().isEmpty();
    }

    QString canonicalXmlElement(const QDomElement &element)
    {
        if (element.isNull())
            return QString();

        QString result = QStringLiteral("<") + element.tagName();

        QStringList attributes;
        const QDomNamedNodeMap attributeMap = element.attributes();
        for (int index = 0; index < attributeMap.count(); ++index) {
            const QDomAttr attribute = attributeMap.item(index).toAttr();
            attributes.append(attribute.name() + QStringLiteral("=") + attribute.value());
        }
        attributes.sort(Qt::CaseSensitive);

        for (const QString &attribute : attributes)
            result += QStringLiteral("|") + attribute;

        result += QStringLiteral(">");

        QDomNode child = element.firstChild();
        while (!child.isNull()) {
            if (child.isElement()) {
                result += canonicalXmlElement(child.toElement());
            } else if (child.isText() || child.isCDATASection()) {
                const QString text = child.nodeValue().trimmed();
                if (!text.isEmpty())
                    result += QStringLiteral("#") + text;
            }
            child = child.nextSibling();
        }

        result += QStringLiteral("</") + element.tagName() + QStringLiteral(">");
        return result;
    }

    bool representationsEquivalent(const QString &left, const QString &right)
    {
        QDomDocument leftDocument;
        QDomDocument rightDocument;
        if (!leftDocument.setContent(left) || !rightDocument.setContent(right))
            return false;

        return canonicalXmlElement(leftDocument.documentElement())
            == canonicalXmlElement(rightDocument.documentElement());
    }

    bool pathsEquivalent(const QString &left, const QString &right)
    {
        if (left.trimmed().isEmpty() || right.trimmed().isEmpty())
            return false;

        QPainterPath leftPath;
        QPainterPath rightPath;
        TupSvg2Qt::svgpath2qtpath(left, leftPath);
        TupSvg2Qt::svgpath2qtpath(right, rightPath);
        return leftPath == rightPath;
    }

    QString currentPathRoute(TupItemResponse *response, NetProject *project)
    {
        if (!response || !project || response->getObjectId().trimmed().isEmpty())
            return QString();

        TupScene *scene = project->sceneAt(response->getSceneIndex());
        TupLayer *layer = scene ? scene->layerAt(response->getLayerIndex()) : nullptr;
        TupFrame *frame = layer ? layer->frameAt(response->getFrameIndex()) : nullptr;
        TupGraphicObject *object = frame
            ? frame->graphicById(response->getObjectId().trimmed())
            : nullptr;
        if (!object)
            return QString();

        QString snapshotError;
        const QString snapshot = TupItemConverter::representationSnapshot(
            object, &snapshotError);
        if (snapshot.trimmed().isEmpty())
            return QString();

        QDomDocument document;
        if (!document.setContent(snapshot))
            return QString();

        const QDomElement root = document.documentElement();
        if (root.tagName() != QStringLiteral("path"))
            return QString();

        return root.attribute(QStringLiteral("coords"));
    }

    QString currentConvertRepresentationPayload(
        TupItemResponse *response,
        NetProject *project)
    {
        if (!response || !project || response->getObjectId().trimmed().isEmpty())
            return QString();

        TupFrame *frame = conversionFrameForResponse(response, project);
        if (!frame)
            return QString();

        TupGraphicObject *object = frame->graphicById(response->getObjectId().trimmed());
        if (!object)
            return QString();

        QString snapshotError;
        const QString snapshot = TupItemConverter::representationSnapshot(
            object, &snapshotError);
        if (snapshot.trimmed().isEmpty())
            return QString();

        const TupProjectRequest request = TupRequestBuilder::createItemRequest(
            response->getSceneIndex(),
            response->getLayerIndex(),
            response->getFrameIndex(),
            response->getItemIndex(),
            response->position(),
            response->spaceMode(),
            response->getItemType(),
            TupProjectRequest::Convert,
            QStringLiteral("path"),
            snapshot.toUtf8(),
            response->getCommandId(),
            QString(),
            response->getObjectId());

        return request.getXml();
    }

    bool prepareAuthoritativeConvertRestore(
        TupItemResponse *response,
        DatabaseHandler *database,
        int dbProjectId,
        int studentId,
        NetProject *project,
        QString *errorCode,
        QString *authoritativeCurrentPayload)
    {
        if (!response || !database || dbProjectId <= 0 || !project)
            return false;

        const QString argument = response->getArg().toString().trimmed();
        const QString sourcePrefix = QStringLiteral("restore_source:");
        const QString targetPrefix = QStringLiteral("restore_target:");

        const bool restoreSource = argument.startsWith(sourcePrefix);
        const bool restoreTarget = argument.startsWith(targetPrefix);
        if (!restoreSource && !restoreTarget)
            return false;

        if (authoritativeCurrentPayload)
            *authoritativeCurrentPayload = currentConvertRepresentationPayload(response, project);

        const QString originalCommandId = argument.mid(
            restoreSource ? sourcePrefix.length() : targetPrefix.length()).trimmed();
        if (originalCommandId.isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("missing_restore_command_id");
            return true;
        }

        const DatabaseHandler::ProjectCommandRecord originalCommand =
            database->getProjectCommand(dbProjectId, originalCommandId);
        if (!originalCommand.found
                || originalCommand.status != QStringLiteral("committed")) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_event_not_found");
            return true;
        }

        if (studentId <= 0 || originalCommand.studentId <= 0
                || originalCommand.studentId != studentId) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_restore_not_owner");
            return true;
        }

        const DatabaseHandler::ProjectEventRecord event =
            database->getProjectEventByCommand(dbProjectId, originalCommandId);
        if (event.commandId.isEmpty()
                || event.eventType != QStringLiteral("item.converted")
                || event.payload.trimmed().isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_event_not_found");
            return true;
        }

        TupRequestParser eventParser;
        if (!eventParser.parse(event.payload.trimmed())) {
            if (errorCode)
                *errorCode = QStringLiteral("invalid_conversion_event");
            return true;
        }

        TupProjectResponse *eventResponse = eventParser.getResponse();
        if (!eventResponse || eventResponse->getPart() != TupProjectRequest::Item
                || eventResponse->originalAction() != TupProjectRequest::Convert) {
            if (errorCode)
                *errorCode = QStringLiteral("invalid_conversion_event");
            return true;
        }

        TupItemResponse *eventItem = static_cast<TupItemResponse *>(eventResponse);
        if (eventItem->getObjectId().trimmed().isEmpty()
                || eventItem->getObjectId().trimmed()
                    != response->getObjectId().trimmed()
                || eventItem->getSceneIndex() != response->getSceneIndex()
                || eventItem->getLayerIndex() != response->getLayerIndex()
                || eventItem->getFrameIndex() != response->getFrameIndex()) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_restore_target_mismatch");
            return true;
        }

        QString sourceRepresentation;
        if (!extractConversionSourceSnapshot(event.payload, &sourceRepresentation)) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_source_snapshot_missing");
            return true;
        }

        const QString targetRepresentation = QString::fromUtf8(eventResponse->getData());
        if (targetRepresentation.trimmed().isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_target_snapshot_missing");
            return true;
        }

        TupFrame *frame = conversionFrameForResponse(response, project);
        TupGraphicObject *object = frame
            ? frame->graphicById(response->getObjectId().trimmed())
            : nullptr;

        if (!object) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_restore_target_missing");
            return true;
        }

        QString snapshotError;
        const QString currentRepresentation = TupItemConverter::representationSnapshot(
            object, &snapshotError);
        if (currentRepresentation.trimmed().isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_restore_snapshot_failed");
            return true;
        }

        const QString expectedCurrentRepresentation = restoreSource
            ? targetRepresentation
            : sourceRepresentation;
        if (!representationsEquivalent(
                currentRepresentation, expectedCurrentRepresentation)) {
            if (errorCode)
                *errorCode = QStringLiteral("conversion_restore_conflict");
            return true;
        }

        const QString representation = restoreSource
            ? sourceRepresentation
            : targetRepresentation;

        response->setArg(QStringLiteral("path"));
        response->setData(representation.toUtf8());
        response->setExternal(true);

        if (errorCode)
            errorCode->clear();
        return true;
    }

    bool prepareAuthoritativeEditNodesRestore(
        TupItemResponse *response,
        DatabaseHandler *database,
        int dbProjectId,
        int studentId,
        NetProject *project,
        QString *errorCode)
    {
        if (!response || !database || dbProjectId <= 0 || !project)
            return false;

        const QString argument = response->getArg().toString().trimmed();
        const QString sourcePrefix = QStringLiteral("restore_source:");
        const QString targetPrefix = QStringLiteral("restore_target:");
        const bool restoreSource = argument.startsWith(sourcePrefix);
        const bool restoreTarget = argument.startsWith(targetPrefix);
        if (!restoreSource && !restoreTarget)
            return false;

        const QString originalCommandId = argument.mid(
            restoreSource ? sourcePrefix.length() : targetPrefix.length()).trimmed();
        if (originalCommandId.isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("missing_restore_command_id");
            return true;
        }

        const DatabaseHandler::ProjectCommandRecord originalCommand =
            database->getProjectCommand(dbProjectId, originalCommandId);
        if (!originalCommand.found || originalCommand.status != QStringLiteral("committed")) {
            if (errorCode)
                *errorCode = QStringLiteral("edit_nodes_event_not_found");
            return true;
        }

        if (studentId <= 0 || originalCommand.studentId <= 0
                || originalCommand.studentId != studentId) {
            if (errorCode)
                *errorCode = QStringLiteral("edit_nodes_restore_not_owner");
            return true;
        }

        const DatabaseHandler::ProjectEventRecord event =
            database->getProjectEventByCommand(dbProjectId, originalCommandId);
        if (event.commandId.isEmpty()
                || event.eventType != QStringLiteral("item.nodes-edited")
                || event.payload.trimmed().isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("edit_nodes_event_not_found");
            return true;
        }

        TupRequestParser eventParser;
        if (!eventParser.parse(event.payload.trimmed())) {
            if (errorCode)
                *errorCode = QStringLiteral("invalid_edit_nodes_event");
            return true;
        }

        TupProjectResponse *eventResponse = eventParser.getResponse();
        if (!eventResponse || eventResponse->getPart() != TupProjectRequest::Item
                || eventResponse->originalAction() != TupProjectRequest::EditNodes) {
            if (errorCode)
                *errorCode = QStringLiteral("invalid_edit_nodes_event");
            return true;
        }

        TupItemResponse *eventItem = static_cast<TupItemResponse *>(eventResponse);
        if (eventItem->getObjectId().trimmed().isEmpty()
                || eventItem->getObjectId().trimmed() != response->getObjectId().trimmed()
                || eventItem->getSceneIndex() != response->getSceneIndex()
                || eventItem->getLayerIndex() != response->getLayerIndex()
                || eventItem->getFrameIndex() != response->getFrameIndex()) {
            if (errorCode)
                *errorCode = QStringLiteral("edit_nodes_restore_target_mismatch");
            return true;
        }

        const QString sourceRoute = QString::fromUtf8(eventResponse->getData());
        const QString targetRoute = eventResponse->getArg().toString();
        if (sourceRoute.trimmed().isEmpty() || targetRoute.trimmed().isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("edit_nodes_snapshot_missing");
            return true;
        }

        const QString currentRoute = currentPathRoute(response, project);
        if (currentRoute.trimmed().isEmpty()) {
            if (errorCode)
                *errorCode = QStringLiteral("edit_nodes_restore_target_missing");
            return true;
        }

        const QString expectedCurrentRoute = restoreSource ? targetRoute : sourceRoute;
        if (!pathsEquivalent(currentRoute, expectedCurrentRoute)) {
            if (errorCode)
                *errorCode = QStringLiteral("edit_nodes_restore_conflict");
            return true;
        }

        response->setArg(restoreSource ? sourceRoute : targetRoute);
        response->setData((restoreSource ? targetRoute : sourceRoute).toUtf8());
        response->setExternal(true);

        if (errorCode)
            errorCode->clear();
        return true;
    }

    QString currentTransformProperties(TupItemResponse *response, NetProject *project)
    {
        if (!response || !project || response->getObjectId().trimmed().isEmpty())
            return QString();

        TupScene *scene = project->sceneAt(response->getSceneIndex());
        TupLayer *layer = scene ? scene->layerAt(response->getLayerIndex()) : nullptr;
        TupFrame *frame = layer ? layer->frameAt(response->getFrameIndex()) : nullptr;
        TupGraphicObject *object = frame
            ? frame->graphicById(response->getObjectId().trimmed())
            : nullptr;
        if (!object || !object->item())
            return QString();

        QDomDocument document;
        document.appendChild(TupSerializer::properties(object->item(), document));
        return document.toString().trimmed();
    }

    bool prepareAuthoritativeTransformRestore(
        TupItemResponse *response, DatabaseHandler *database, int dbProjectId,
        int studentId, NetProject *project, QString *errorCode)
    {
        if (!response || !database || dbProjectId <= 0 || !project)
            return false;

        const QString argument = response->getArg().toString().trimmed();
        const QString sourcePrefix = QStringLiteral("restore_source:");
        const QString targetPrefix = QStringLiteral("restore_target:");
        const bool restoreSource = argument.startsWith(sourcePrefix);
        const bool restoreTarget = argument.startsWith(targetPrefix);
        if (!restoreSource && !restoreTarget)
            return false;

        const QString originalCommandId = argument.mid(
            restoreSource ? sourcePrefix.length() : targetPrefix.length()).trimmed();
        if (originalCommandId.isEmpty()) {
            if (errorCode) *errorCode = QStringLiteral("missing_restore_command_id");
            return true;
        }

        const DatabaseHandler::ProjectCommandRecord originalCommand =
            database->getProjectCommand(dbProjectId, originalCommandId);
        if (!originalCommand.found || originalCommand.status != QStringLiteral("committed")) {
            if (errorCode) *errorCode = QStringLiteral("transform_event_not_found");
            return true;
        }
        if (studentId <= 0 || originalCommand.studentId <= 0
                || originalCommand.studentId != studentId) {
            if (errorCode) *errorCode = QStringLiteral("transform_restore_not_owner");
            return true;
        }

        const DatabaseHandler::ProjectEventRecord event =
            database->getProjectEventByCommand(dbProjectId, originalCommandId);
        if (event.commandId.isEmpty() || event.eventType != QStringLiteral("item.transformed")
                || event.payload.trimmed().isEmpty()) {
            if (errorCode) *errorCode = QStringLiteral("transform_event_not_found");
            return true;
        }

        TupRequestParser eventParser;
        if (!eventParser.parse(event.payload.trimmed())) {
            if (errorCode) *errorCode = QStringLiteral("invalid_transform_event");
            return true;
        }
        TupProjectResponse *eventResponse = eventParser.getResponse();
        if (!eventResponse || eventResponse->getPart() != TupProjectRequest::Item
                || eventResponse->originalAction() != TupProjectRequest::Transform) {
            if (errorCode) *errorCode = QStringLiteral("invalid_transform_event");
            return true;
        }

        TupItemResponse *eventItem = static_cast<TupItemResponse *>(eventResponse);
        if (eventItem->getObjectId().trimmed().isEmpty()
                || eventItem->getObjectId().trimmed() != response->getObjectId().trimmed()
                || eventItem->getSceneIndex() != response->getSceneIndex()
                || eventItem->getLayerIndex() != response->getLayerIndex()
                || eventItem->getFrameIndex() != response->getFrameIndex()) {
            if (errorCode) *errorCode = QStringLiteral("transform_restore_target_mismatch");
            return true;
        }

        const QString sourceProperties = QString::fromUtf8(eventResponse->getData()).trimmed();
        const QString targetProperties = eventResponse->getArg().toString().trimmed();
        if (sourceProperties.isEmpty() || targetProperties.isEmpty()) {
            if (errorCode) *errorCode = QStringLiteral("transform_snapshot_missing");
            return true;
        }

        const QString currentProperties = currentTransformProperties(response, project);
        if (currentProperties.isEmpty()) {
            if (errorCode) *errorCode = QStringLiteral("transform_restore_target_missing");
            return true;
        }

        const QString expectedCurrent = restoreSource ? targetProperties : sourceProperties;
        if (!representationsEquivalent(currentProperties, expectedCurrent)) {
            if (errorCode) *errorCode = QStringLiteral("transform_restore_conflict");
            return true;
        }

        response->setArg(restoreSource ? sourceProperties : targetProperties);
        response->setData(expectedCurrent.toUtf8());
        response->setExternal(true);
        if (errorCode) errorCode->clear();
        return true;
    }

}

// QString ProjectManager::BROWSER_FINGERPRINT = QString("TupiTube_Media 1.0");

ProjectManager::ProjectManager() : Observer()
{
    m_dbHandler = new DatabaseHandler();
    loadVideoPlugin();
}

ProjectManager::~ProjectManager()
{
    for (CommandResultRegistry *registry : m_commandResultRegistries)
        delete registry;

    m_commandResultRegistries.clear();
}

void ProjectManager::createProject(Connection *connection)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::createProject()]";
    #endif
   
    int studentID = connection->student()->uid(); 
    QString projectName = m_connectionData.name();

    QList<int> studentList;
    studentList.insert(0, studentID);


    NetProject *project = new NetProject;
    project->setProjectParams(studentID);
    project->setProjectName(projectName);
    project->setAuthor(m_connectionData.author());
    project->setDescription(m_connectionData.description());
    project->setCurrentBgColor(m_connectionData.bgColor());
    project->setDimension(m_connectionData.dimension());
    project->setFPS(m_connectionData.fps());
    project->setStudents(studentList);

    // --- Auto-select valid Period for current date ---
    QList<DatabaseHandler::PeriodInfo> periods = m_dbHandler->getAllPeriods();
    QDate currentDate = QDate::currentDate();
    int selectedPeriodId = -1;
    for (const DatabaseHandler::PeriodInfo &period : periods) {
        QDate start = QDate::fromString(period.startDate, "yyyy-MM-dd");
        QDate end = QDate::fromString(period.endDate, "yyyy-MM-dd");
        if (start.isValid() && end.isValid() && currentDate >= start && currentDate <= end) {
            selectedPeriodId = period.periodId;
            break;
        }
    }
    if (selectedPeriodId != -1) {
        project->setProperty("period_id", selectedPeriodId);
    } else {
        // No valid period found for today, handle as needed (error, fallback, etc.)
        connection->sendNotification(300, QObject::tr("No valid Period found for today's date. Cannot create project."), Notification::Error);
        connection->close();
        delete project;
        return;
    }

    // --- Resolve class_id from the student record ---
    int studentClassId = -1;
    QSqlQuery classQuery;
    classQuery.prepare("SELECT class_id FROM tupitube_student WHERE student_id = ?");
    classQuery.addBindValue(studentID);
    if (classQuery.exec() && classQuery.next()) {
        studentClassId = classQuery.value(0).toInt();
    }
    if (studentClassId == -1) {
        connection->sendNotification(300, QObject::tr("No class assigned to this student. Cannot create project."), Notification::Error);
        connection->close();
        delete project;
        return;
    }
    project->setProperty("class_id", studentClassId);

    bool dbSuccess = false;
    bool saved = project->save();
    if (saved) {
        dbSuccess = m_dbHandler->addProject(project);
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[ProjectManager::createProject()] - Fatal Error: Project can't be saved to filesystem -> " << project->filename();
        #endif
        connection->sendNotification(300, QObject::tr("Can't create project \"%1\"").arg(projectName),
                                     Notification::Error);
        connection->close();
        return;
    }

    if (dbSuccess) {
        QObject::connect(project, SIGNAL(requestSendMessage(int, const QString&, Notification::Level)),
                         connection, SLOT(sendNotification(int, const QString&, Notification::Level)));
        QString uid = QString::number(studentID);
        QString filename = project->filename();
        QString cacheDir = CACHE_DIR;
        if (cacheDir.endsWith("/"))
            cacheDir.chop(1);
        project->setDataDir(cacheDir + "/" + uid + "/" + filename);
        registerProject(connection, uid, filename, project);

        QString msg = QObject::tr("New project \"%1\" has been created by student %2").arg(projectName, connection->student()->login());
        Logger::self()->info(msg);
        emit projectEventLog(msg, "INFO");

    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[ProjectManager::createProject()] - Fatal Error: Project record can't be saved to database -> " << project->filename();
        #endif
        connection->sendNotification(300, QObject::tr("Cannot create project \"%1\"").arg(projectName),
                                     Notification::Warning);
        connection->close();
        return;
    }
}

void ProjectManager::openProject(const QString &filename, const QString &owner, Connection *connection, bool sendSnapshot)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::openProject()]";
        qWarning() << "[ProjectManager::openProject()] - Opening project: " << filename;
    #endif

    QString ownerID = m_dbHandler->studentID(owner);
    QString projectID = m_dbHandler->exists(filename, ownerID);

    if (projectID.compare("-1") != 0) {
        if (!m_dbHandler->accessIsConfirmed(projectID, connection->student()->uid())) {
            #ifdef TUP_DEBUG
                   qDebug() << "[ProjectManager::openProject()] - Fatal Error: Insufficient permissions to access project -> " << filename;
                   qDebug() << "[ProjectManager::openProject()] - Request made by " << connection->student()->login();
            #endif
            connection->sendNotification(360, QObject::tr("Insufficient Permissions"),
                                         Notification::Error);
            connection->close();
            return;
        }


        const DatabaseHandler::ProjectRevisionInfo revisionInfo =
            m_dbHandler->getProjectRevisionInfo(projectID.toInt());
        if (!revisionInfo.found
                || revisionInfo.currentRevision != revisionInfo.snapshotRevision) {
            qCritical()
                << "[ProjectManager::openProject()]"
                << "Authoritative revision metadata is inconsistent. Project:" << filename
                << "Current revision:" << revisionInfo.currentRevision
                << "Snapshot revision:" << revisionInfo.snapshotRevision;
            connection->sendNotification(323, QObject::tr("Error while recovering project %1").arg(filename),
                                         Notification::Error);
            connection->close();
            return;
        }

        FileManager recoveryManager;
        if (!recoveryManager.reconcileAuthoritativeSnapshot(
                filename, ownerID, revisionInfo.snapshotRevision, revisionInfo.snapshotChecksum)) {
            qCritical()
                << "[ProjectManager::openProject()]"
                << "Unable to reconcile the authoritative server snapshot. Project:" << filename
                << "Revision:" << revisionInfo.snapshotRevision;
            connection->sendNotification(323, QObject::tr("Error while recovering project %1").arg(filename),
                                         Notification::Error);
            connection->close();
            return;
        }

        NetProject *project = new NetProject;
        QObject::connect(project, SIGNAL(requestSendMessage(int, const QString&, Notification::Level)),
                         connection, SLOT(sendNotification(int, const QString&, Notification::Level)));

        // Lookup project title from database
        QString projectTitle;
        {
            QSqlQuery query;
            query.prepare("SELECT title FROM tupitube_project WHERE filename = :filename AND student_id = :student_id");
            query.bindValue(":filename", filename);
            query.bindValue(":student_id", ownerID);
            if (query.exec() && query.next()) {
                projectTitle = query.value(0).toString();
            } else {
                projectTitle = filename; // fallback
            }
        }

        if (!m_openedProjects.contains(filename)) {
            FileManager *manager = new FileManager;
            bool ok = manager->load(filename, project, ownerID);

            if (!ok) {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::openProject()] - Fatal Error: Can't load project -> " << filename;
                #endif
                connection->sendNotification(323, QObject::tr("Error while loading project %1").arg(filename), 
                                             Notification::Error);
                connection->close();
                return;
            }

            project->setOwner(ownerID.toInt());
            project->setFilename(filename);

            // Loading a legacy project may synthesize persistent object IDs for
            // native objects whose serialized <object> nodes predate object_id.
            // Persist that migration before any collaborative snapshot is sent so
            // every participant receives the same server-authoritative identity.
            if (!project->save(true)) {
                qCritical()
                    << "[ProjectManager::openProject()]"
                    << "Unable to persist authoritative object identity migration for project:"
                    << filename;
                connection->sendNotification(323, QObject::tr("Error while preparing project %1").arg(filename),
                                             Notification::Error);
                connection->close();
                return;
            }

            QString msg = QObject::tr("Student %1 from %2 opened project: %3")
                .arg(connection->student()->login(), connection->ip(), projectTitle);
            Logger::self()->info(msg);
            emit projectEventLog(msg, "INFO");
        } else {
            #ifdef TUP_DEBUG
                   qWarning() << "[ProjectManager::openProject()] - Project is already open - Connecting socket...";
            #endif

            project = m_openedProjects.value(filename);
            project->save();

            QObject::connect(project, SIGNAL(requestSendMessage(int, const QString&, Notification::Level)),
                             connection, SLOT(sendNotification(int, const QString&, Notification::Level)));

            QString msg = QObject::tr("Student %1 from %2 opened project: %3")
                .arg(connection->student()->login(), connection->ip(), projectTitle);
            Logger::self()->info(msg);
            emit projectEventLog(msg, "INFO");
        }

        registerProject(connection, ownerID, filename, project, sendSnapshot);
    } else {
        #ifdef TUP_DEBUG
               qDebug() << "[ProjectManager::openProject()] - Fatal Error: Project doesn't exist -> " << filename;
        #endif

        connection->sendNotification(321, QObject::tr("Project %1 doesn't exist").arg(filename),
                                     Notification::Error);
        connection->close();
    }
}

void ProjectManager::importProject(Connection *connection, const QString &path, const QByteArray &data)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::importProject()]";
    #endif

    if (data.size() > 0) {
        int studentID = connection->student()->uid();
        QString uid = QString::number(studentID);

        NetProject *project = new NetProject;
        project->setProjectParams(studentID);
        QString filename = project->filename();

        QString absolutePath = kAppProp->repositoryDir() + uid + "/projects/" + filename + ".tup";
        QFile file(absolutePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();

            FileManager *manager = new FileManager;
            bool ok  = manager->load(filename, project, uid);
            if (!ok) {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::importProject()] - Fatal Error: Can't load project -> " << filename;
                #endif
                connection->sendNotification(324, QObject::tr("Error while importing project %1").arg(path),
                                             Notification::Error);
                connection->close();
                return;
            }

            bool dbSuccess = m_dbHandler->addProject(project);

            if (!dbSuccess) {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::importProject()] - Fatal Error: Project record can't be saved to database -> " << filename;
                #endif
                connection->sendNotification(300, QObject::tr("Cannot create project \"%1\" in server").arg(project->getName()), 
                                             Notification::Error);
                connection->close();
                return;
            }

            QObject::connect(project, SIGNAL(requestSendMessage(int, const QString&, Notification::Level)),
                             connection, SLOT(sendNotification(int, const QString&, Notification::Level)));
            Logger::self()->info(QObject::tr("Project \"%1\" has been imported by student %2").arg(project->getName(),
                                       connection->student()->login()));

            registerProject(connection, uid, filename, project);
        } else {
           #ifdef TUP_DEBUG
                  qDebug() << "[ProjectManager::importProject()] - Fatal Error: Can't save project to filesystem -> " << filename;
           #endif
           connection->sendNotification(300, QObject::tr("Cannot create project \"%1\" in server").arg(project->getName()),
                                        Notification::Error);
           connection->close();
           return;
        }
    } else {
        #ifdef TUP_DEBUG
               qDebug() << "[ProjectManager::importProject()] - Fatal Error: Project file is empty -> " << path;
        #endif
        connection->sendNotification(300, QObject::tr("Cannot create project \"%1\" in server").arg(path),
                                     Notification::Error);
        connection->close();
        return;
    }
}

void ProjectManager::registerProject(Connection *connection, const QString &uid, const QString &filename, NetProject *project, bool sendSnapshot)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::registerProject()]";
        qWarning() << "[ProjectManager::registerProject()] - Registering project with ID: " << filename;
    #endif

    project->setOpen(true);

    connection->setData(Info::ProjectID, filename);
    connection->setData(Info::ProjectIsOpen, true);
    m_openedProjects.insert(filename, project);

    if (!m_commandResultRegistries.contains(filename))
        m_commandResultRegistries.insert(filename, new CommandResultRegistry);

    QString absolutePath = kAppProp->repositoryDir() + "/projects/" + uid + "/sources/" + filename + "/" + filename + ".tup";

    // Get project ID from database
    QString projectId = m_dbHandler->exists(filename, uid);
    
    // Build loginList from ALL project collaborators in database (not just connected students)
    QStringList studentList;
    
    // Add project owner
    QString ownerStudentname = m_dbHandler->getOwnerStudentname(projectId.toInt());
    studentList << ownerStudentname;
    
    // Add all collaborators from database
    QList<DatabaseHandler::CollaboratorInfo> collaborators = m_dbHandler->getProjectCollaborators(projectId.toInt());
    for (const DatabaseHandler::CollaboratorInfo &collab : collaborators) {
        if (!studentList.contains(collab.studentname))
            studentList << collab.studentname;
    }
    
    QString loginList = studentList.join(",");

    #ifdef TUP_DEBUG
        qWarning() << "[ProjectManager::registerProject()] - Login list: " << loginList;
    #endif

    if (sendSnapshot) {
        Project projectPackage(loginList, absolutePath);
        connection->sendStringToClient(projectPackage.toString());

        // The snapshot must carry an authoritative ordering baseline. The
        // legacy server_project package has no revision field, so send a small
        // companion packet immediately after it. This also prevents a client
        // that disconnects before seeing any live project_event from syncing
        // with last_revision=-1.
        const DatabaseHandler::ProjectRevisionInfo revisionInfo =
            m_dbHandler->getProjectRevisionInfo(projectId.toInt());
        if (revisionInfo.found) {
            QDomDocument revisionDocument;
            QDomElement revisionRoot =
                revisionDocument.createElement(QStringLiteral("project_revision"));
            revisionRoot.setAttribute(QStringLiteral("version"), QStringLiteral("1"));
            revisionRoot.setAttribute(QStringLiteral("project_id"), filename);
            revisionRoot.setAttribute(
                QStringLiteral("revision"), revisionInfo.currentRevision);
            revisionRoot.setAttribute(
                QStringLiteral("saved_revision"), revisionInfo.savedRevision);
            revisionRoot.setAttribute(
                QStringLiteral("event_index"),
                revisionInfo.currentRevision > 0 ? 0 : -1);
            revisionDocument.appendChild(revisionRoot);
            connection->sendStringToClient(revisionDocument.toString(0));
        }
    }

    const bool alreadyConnected = m_connectionList[filename].contains(connection);
    if (!alreadyConnected) {
        // Send notice to connected partners that new student joined.
        QList<Connection *> partners = m_connectionList[filename];
        int size = partners.size();
        Notice msg(connection->student()->login(), 1);

        for (int i = 0; i < size; ++i)
             partners.at(i)->sendStringToClient(msg.toString());

        // Send notices to the new student about already-connected partners.
        for (int i = 0; i < size; ++i) {
            Notice onlineMsg(partners.at(i)->student()->login(), 1);
            connection->sendStringToClient(onlineMsg.toString());
        }

        m_connectionList[filename].append(connection);
        emit projectRegistered(filename);
    }
}

void ProjectManager::createImage(Connection *connection, int frame, int scene, const QString &title, const QString &topics, const QString &description)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::createImage()]";
    #endif

    Q_UNUSED(connection)
    Q_UNUSED(frame)
    Q_UNUSED(scene)
    Q_UNUSED(title)
    Q_UNUSED(topics)
    Q_UNUSED(description)

    /*
    QString projectID = connection->data(Info::ProjectID).toString();
    NetProject *project = m_openedProjects.value(projectID);
    QString uid = QString::number(connection->student()->uid());
    QString code = project->fileCode();
    QSize dimension = project->getDimension();

    QString path = kAppProp->repositoryDir() + "students/" + uid + "/images/" + code + ".png";
    GenericExportPlugin exporter;
    bool imageOk = exporter.exportFrame(frame, project->getBgColor(), path, project->sceneAt(scene), dimension);

    if (imageOk) {
        QPixmap *pixmap = new QPixmap();
        bool isOk = pixmap->load(path);
        bool portrait = true;

        if (isOk) {
            QPixmap newpix;

            if (dimension.width() >= dimension.height()) {
                portrait = false;
                newpix = QPixmap(pixmap->scaledToWidth(200, Qt::SmoothTransformation));
                if (dimension.width() > 720) {
                    QPixmap image;
                    image = QPixmap(pixmap->scaledToWidth(720, Qt::SmoothTransformation));
                    image.save(path);
                }
            } else {
                newpix = QPixmap(pixmap->scaledToHeight(200, Qt::SmoothTransformation));
                if (dimension.height() > 800) {
                    QPixmap image;
                    image = QPixmap(pixmap->scaledToHeight(800, Qt::SmoothTransformation));
                    image.save(path);
                }
            }

           QString thumb = kAppProp->repositoryDir() + "students/" + uid + "/images/thumbnails/" + code + ".png";
           newpix.save(thumb);
        } else {
           #ifdef TUP_DEBUG
                  qDebug() << "ProjectManager::createImage() - [ Fatal Error ] - Error creating thumbnail for Image -> " << code;
           #endif
           return;
        }

        bool dbSuccess = m_dbHandler->addWork(projectID, QString("image"), uid, title, topics, description, code, portrait);
        if (!dbSuccess) {
            #ifdef TUP_DEBUG
                   qDebug() << "[ProjectManager::createImage()] - Fatal Error: Image record can't be saved to database -> " << code;
            #endif
            connection->sendNotification(382, QObject::tr("Error while posting image \"%1\"").arg(title), Notification::Error);
            return;
        }

        QString login = connection->student()->login();
        Logger::self()->info(QObject::tr("Image %2.png has been exported by student \"%1\"").arg(login, code));
        connection->sendNotification(100, QObject::tr("Image \"%1\" posted successfully").arg(title), Notification::Info);
    } else {
        #ifdef TUP_DEBUG
               qDebug() << "[ProjectManager::createImage()] - Fatal Error: Can't export image " << code << " from project " << projectID;
        #endif
        connection->sendNotification(382, QObject::tr("Error while posting image \"%1\"").arg(title), Notification::Error);
    }
    */
}

void ProjectManager::createVideo(Connection *connection, const QString &title, const QString &topics, const QString &desc, int fps, const QList<int> scenes)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::createVideo()]";
    #endif

    Q_UNUSED(connection)
    Q_UNUSED(title)
    Q_UNUSED(topics)
    Q_UNUSED(desc)
    Q_UNUSED(fps)
    Q_UNUSED(scenes)

    /*
    QString projectID = connection->data(Info::ProjectID).toString();
    QString uid = QString::number(connection->student()->uid());
    NetProject *project = m_openedProjects.value(projectID);

    if (project) {
        QString code = project->fileCode();
        QSize dimension = project->getDimension();
        bool isOk = false; 
        int thumbScene = 0;
        int thumbFrame = 0;

        if (scenes.count() > 0) {
            QList<TupScene*> sceneList;
            int middleScene = scenes.count()/2;
            for (int i=0; i < scenes.count(); i++) {
                 int sceneIndex = scenes.at(i);

                 if (scenes.at(i) < project->scenesCount()) {
                     TupScene *scene = project->sceneAt(sceneIndex);
                     if (scene) {
                         sceneList.append(scene);
                         if (middleScene == i) {
                             thumbScene = sceneIndex;
                             thumbFrame = scene->framesCount()/2;
                         }
                     }
                 } else {
                     #ifdef TUP_DEBUG
                            qDebug() << "[ProjectManager::createVideo()] - Fatal Error: Scene index is Invalid: " << scenes.at(i);
                     #endif
                 }
            }

            if (sceneList.count() > 0) {
                QString base = kAppProp->repositoryDir() + "students/" + uid + "/animations/" + code;

                QString fileName = base + ".mp4";
		isOk = m_exporter->exportToFormat(project->getBgColor(), fileName, sceneList, TupExportInterface::MP4, dimension, dimension, fps);
                // isOk = m_exporter->exportToFormat(project->getBgColor(), fileName, sceneList, TupExportInterface::WEBM, dimension, dimension, fps);

                // fileName = base + ".swf";
                // isOk = isOk && m_exporter->exportToFormat(project->bgColor(), fileName, sceneList, TupExportInterface::SWF, dimension, dimension, fps);
            }
        }

        if (isOk) {
            QString orientation = "1";
            bool portrait = (dimension.width() > dimension.height()) ? false : true;

            bool dbSuccess = m_dbHandler->addWork(projectID, QString("animation"), uid, title, topics, desc, code, portrait);
            if (!dbSuccess) {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::createVideo()] - Fatal Error: Video record can't be saved to database -> " << code;
                #endif
                connection->sendNotification(383, QObject::tr("Error while posting animation \"%1\"").arg(title), Notification::Error);
                return;
            }

            QString fileName = kAppProp->repositoryDir() + "students/" + uid + "/animations/thumbnails/" + code + ".png";
            GenericExportPlugin exporter;
        
            bool imageOk = exporter.exportFrame(thumbFrame, project->getBgColor(), fileName, project->sceneAt(thumbScene), dimension);

            if (imageOk) {
                QPixmap *pixmap = new QPixmap();
                bool isOk = pixmap->load(fileName);
                if (isOk) {
                    QPixmap newpix;
                    if (dimension.width() >= dimension.height())
                        newpix = QPixmap(pixmap->scaledToWidth(200, Qt::SmoothTransformation));
                    else
                        newpix = QPixmap(pixmap->scaledToHeight(200, Qt::SmoothTransformation));

                    newpix.save(fileName);
                } else { 
                    #ifdef TUP_DEBUG
                           qDebug() << "[ProjectManager::createVideo()] - Fatal Error: Can't resize thumbnail " << code << " from project " << projectID;
                    #endif
                    connection->sendNotification(383, QObject::tr("Error while posting animation \"%1\"").arg(title), Notification::Error);
                    return;
                }
            } else {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::createVideo()] - Fatal Error: Can't create thumbnail " << code << " from project " << projectID;
                #endif
                connection->sendNotification(383, QObject::tr("Error while posting animation \"%1\"").arg(title), Notification::Error);
                return;
            }

            QString login = connection->student()->login();
            Logger::self()->info(QObject::tr("Video %2.webm has been exported by student \"%1\"").arg(login, code));
            connection->sendNotification(101, QObject::tr("Video \"%1\" posted successfully").arg(title), Notification::Info);
        } else {
            #ifdef TUP_DEBUG
                   qDebug() << "[ProjectManager::createVideo()] - Fatal Error: Video record can't be exported -> " << code;
            #endif
            connection->sendNotification(383, QObject::tr("Error while posting animation \"%1\"").arg(title), Notification::Error);
        }
    } else {
        #ifdef TUP_DEBUG
               qDebug() << "[ProjectManager::createVideo()] - Fatal Error: Project pointer is NULL -> " << projectID;    
        #endif
        connection->sendNotification(383, QObject::tr("Error while posting animation \"%1\"").arg(title), Notification::Error);
    }
    */
}

void ProjectManager::createStoryboard(Connection *connection, int sceneIndex)
{
    Q_UNUSED(connection)
    Q_UNUSED(sceneIndex)

    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::createStoryboard()]";
    #endif

    /*
    QString projectID = connection->data(Info::ProjectID).toString();
    QString uid = QString::number(connection->student()->uid());
    QString login = connection->student()->login();

    NetProject *project = m_openedProjects.value(projectID);

    if (project) {
        QString directory = project->fileCode();
        TupScene *scene = project->sceneAt(sceneIndex);
        TupStoryboard *storyboard = new TupStoryboard(login);
        QString title = "";
        QString topics = "";

        if (scene) {
            storyboard = project->sceneAt(sceneIndex)->storyboardStructure(); 
            title = storyboard->storyTitle();
            topics = storyboard->storyTopics();
            bool dbSuccess = m_dbHandler->addStoryboard(projectID, uid, title, topics, storyboard->storySummary(), directory);

            if (!dbSuccess) {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Storyboard record " << title << " can't be saved to database from project " << projectID;
                #endif
                connection->sendNotification(384, QObject::tr("Error while posting storyboard \"%1\"").arg(title), Notification::Error);
                return;
            }
        } else {
            #ifdef TUP_DEBUG
                   qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Can't export storyboard. Scene " << sceneIndex << " is unavailable from project " << projectID;
            #endif
            connection->sendNotification(384, QObject::tr("Error while posting storyboard"), Notification::Error);
            return;
        }

        QString storyID = m_dbHandler->storyboardID(uid, directory);

        QString absolutePath = kAppProp->repositoryDir() + "students/" + uid + "/storyboards/" + directory;
        QDir repository(absolutePath);
        bool ok = repository.mkdir(absolutePath);

        if (!ok) {
            #ifdef TUP_DEBUG
                   qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Insufficient permissions to create directory -> " << absolutePath;
            #endif
            return;
        }

        QColor bgColor = project->getBgColor();
        QSize dimension = project->getDimension();

        for (int i=0; i < scene->framesCount(); i++) {
             QString frame = QString::number(i);
             QString fileName = project->fileCode();
             QString path = absolutePath + "/" + fileName + ".png"; 
             GenericExportPlugin exporter;
             bool imageOk = exporter.exportFrame(i, bgColor, path, scene, dimension);

             if (imageOk) {
                 QPixmap *pixmap = new QPixmap();
                 bool isOk = pixmap->load(path);
                 if (isOk) {
                     if (dimension.width() >= dimension.height()) {
                         if (dimension.width() > 720) {
                             QPixmap image;
                             image = QPixmap(pixmap->scaledToWidth(720, Qt::SmoothTransformation));
                             image.save(path);
                         }
                     } else {
                         if (dimension.height() > 800) {
                             QPixmap image;
                             image = QPixmap(pixmap->scaledToHeight(800, Qt::SmoothTransformation));
                             image.save(path);
                         }
                     }
                 } else {
                     #ifdef TUP_DEBUG
                            qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Error resizing frame #" << i << " for Storyboard -> " << storyID;
                     #endif
                     return;
                 }

                 if (i == 0) {
                     QPixmap *pixmap = new QPixmap();
                     bool isOk = pixmap->load(path);

                     if (isOk) {
                         QPixmap newpix;
                         if (dimension.width() >= dimension.height())
                             newpix = QPixmap(pixmap->scaledToWidth(200, Qt::SmoothTransformation));
                         else
                             newpix = QPixmap(pixmap->scaledToHeight(200, Qt::SmoothTransformation));

                         QString thumb = kAppProp->repositoryDir() + "students/" + uid + "/storyboards/thumbnails/" + directory + ".png";
                         newpix.save(thumb);
                      } else {
                         #ifdef TUP_DEBUG
                                qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Error creating main thumbnail for Storyboard -> " << storyID;
                         #endif
                         return;
                      }
                 }

                 QString frameTitle = storyboard->sceneTitle(i);
                 if (frameTitle.length() == 0)
                     frameTitle = "Frame No " + frame;

                 QString frameDescription = storyboard->sceneDescription(i);
                 if (frameDescription.length() == 0)
                     frameDescription = "No Description";

                 QString frameDuration = storyboard->sceneDuration(i);
                 if (frameDuration.length() == 0)
                     frameDuration = "Undefined";

                 bool dbSuccess = m_dbHandler->addStoryFrame(storyID, projectID, uid, frameTitle, topics, frameDescription, fileName, frameDuration);
                 if (!dbSuccess) {
                     #ifdef TUP_DEBUG
                            qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Frame record can't be saved to database -> " << fileName;
                     #endif
                     connection->sendNotification(384, QObject::tr("Error while posting storyboard \"%1\"").arg(title), Notification::Error);
                     return;
                 }
             }  else {
                 #ifdef TUP_DEBUG
                        qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Can't export frame " << fileName << " from project " << projectID;
                 #endif
                 connection->sendNotification(384, QObject::tr("Error while posting frame \"%1\" from storyboard").arg(fileName), Notification::Error);
                 return;
             }
        }

        Logger::self()->info(QObject::tr("Storyboard \"%1\" has been exported by student \"%2\"").arg(title, login));
        connection->sendNotification(102, QObject::tr("Storyboard \"%1\" posted successfully").arg(title), Notification::Info);

    } else {
        #ifdef TUP_DEBUG
               qDebug() << "[ProjectManager::createStoryboard()] - Fatal Error: Can't open project " << projectID;
        #endif
        connection->sendNotification(384, QObject::tr("Error while posting storyboard. Project unavailable"), Notification::Error);
        return;
    }
    */
}

void ProjectManager::updateStoryboard(Connection *connection, int sceneIndex, const QString &storyXml)
{
    Q_UNUSED(connection)
    Q_UNUSED(sceneIndex)
    Q_UNUSED(storyXml)

    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::updateStoryboard()]";
    #endif

    /*
    QString projectID = connection->data(Info::ProjectID).toString();
    NetProject *project = m_openedProjects.value(projectID);

    if (project) {
        TupStoryboard *storyboard = new TupStoryboard(connection->student()->login());
        storyboard->fromXml(storyXml);

        project->sceneAt(sceneIndex)->setStoryboard(storyboard); 

        saveProject(projectID, true);
        connection->sendNotification(102, QObject::tr("Storyboard \"%1\" posted successfully").arg(storyboard->storyTitle()), Notification::Info);
    }
    */
}

void ProjectManager::handlePackage(PackageBase *const pkg)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::handlePackage()]";
    #endif

    QString root = pkg->root();
    QString package = pkg->xml();
    Connection *connection = pkg->source();

    if (root == "project_request") {
        #ifdef TUP_DEBUG
            qWarning() << "[ProjectManager::handlePackage()] - project_request from:" << connection->student()->login();
        #endif
        if (connection->student()->isEnabled()) {
            if (!connection->data(Info::ProjectID).toString().isNull()) {
                QString projectID = connection->data(Info::ProjectID).toString();
                #ifdef TUP_DEBUG
                    qWarning() << "[ProjectManager::handlePackage()] - Processing request for project:" << projectID;
                #endif

                const ProjectCommandResult result =
                    handleProjectRequest(projectID, package, connection->student()->uid());

                sendCommandResult(connection, result);

                if (result.duplicate) {
#ifdef TUP_DEBUG
                    qDebug()
                        << "[ProjectManager::handlePackage()]"
                        << "Stored command result returned without rebroadcast."
                        << "Command:" << result.commandId;
#endif
                } else if (result.isCommitted()) {
#ifdef TUP_DEBUG
                    qDebug()
                        << "[ProjectManager::handlePackage()]"
                        << "Authoritative event persisted."
                        << "Event:" << result.eventId
                        << "Type:" << result.eventType
                        << "Revision:" << result.committedRevision;
#endif

                    sendProjectEventToProjectMembers(connection, result);
                } else {
#ifdef TUP_DEBUG
                    qWarning()
                        << "[ProjectManager::handlePackage()]"
                        << "Command was not committed."
                        << "Command:" << result.commandId
                        << "Error:" << result.errorCode;
#endif
                }
            } else {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Project ID undefined";
                #endif
            }

        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[ProjectManager::handlePackage()] - Student NOT enabled:" << connection->student()->login();
            #endif
            connection->sendNotification(360, QObject::tr("Insufficient permissions"), 
                                         Notification::Warning);
        }
    } else if (root == QStringLiteral("project_sync_request")) {
        handleProjectSyncRequest(connection, package);
    } else if (root == "project_open") {
               if (connection->student()->isEnabled()) {
                   OpenProjectParser parser;
                   if (parser.parse(package)) {
                       openProject(parser.projectID(), parser.owner(), connection);
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), 
                                                Notification::Warning);
               }
    } else if (root == "project_new") {
               if (connection->student()->isEnabled()) {
                   if (m_connectionData.parse(package)) {
                       createProject(connection);
                   } else {
                       connection->sendNotification(360, QObject::tr("Insufficient permissions"), 
                                                    Notification::Warning);
                   }
               } else {
                   #ifdef TUP_DEBUG
                          qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Insufficient permissions to create project";
                   #endif
               }
    } else if (root == "project_import") {
               if (connection->student()->isEnabled()) {
                   ImportProjectParser parser;
                   if (parser.parse(package))
                       importProject(connection, parser.path(), parser.data());
               } else {
                   #ifdef TUP_DEBUG
                          qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Insufficient permissions to import project";
                   #endif
               }
    } else if (root == "project_list") {
               connection->setData(Info::ProjectIsOpen, false);
               if (connection->student()->isEnabled()) {
                   ListProjectsParser parser;
                   if (parser.parse(package)) {
                       listStudentProjects(connection);
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_list package";
                       #endif
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), Notification::Warning);
               }
    } else if (root == "project_save") {
               if (connection->student()->isEnabled()) {
                   SaveProjectParser parser;
                   if (parser.parse(package)) {
                       QString projectID = connection->data(Info::ProjectID).toString();
                       if (m_openedProjects.contains(projectID)) {
                           // Save quietly so NetProject does not broadcast a generic
                           // notification to every connection. The client that issued
                           // project_save needs a deterministic completion response.
                           if (saveProject(projectID, true)) {
                               const int dbProjectId = m_dbHandler->getProjectIdFromFilename(projectID);
                               qint64 savedRevision = -1;
                               if (dbProjectId <= 0
                                       || !m_dbHandler->markProjectSaved(dbProjectId, &savedRevision)) {
                                   connection->sendNotification(381,
                                       QObject::tr("Project file was saved, but save revision metadata could not be updated"),
                                       Notification::Error);
                               } else {
                                   // Acknowledge the save only to the requester so only that
                                   // client completes its local save UI state.
                                   connection->sendNotification(380, QObject::tr("Project saved successfully"),
                                                                Notification::Info);

                                   // Inform active collaborators visually and also send a
                                   // machine-readable saved revision. The latter lets clients
                                   // resolve dirty state without depending on a transient OSD
                                   // notification, and is safe even if edits continue later.
                                   const QString saverLogin = connection->student()->login();
                                   const QString collaboratorMessage =
                                       QObject::tr("%1 saved the project").arg(saverLogin);

                                   QDomDocument savedDocument;
                                   QDomElement savedRoot =
                                       savedDocument.createElement(QStringLiteral("project_saved"));
                                   savedRoot.setAttribute(QStringLiteral("version"), QStringLiteral("1"));
                                   savedRoot.setAttribute(QStringLiteral("project_id"), projectID);
                                   savedRoot.setAttribute(QStringLiteral("revision"), savedRevision);
                                   savedRoot.setAttribute(QStringLiteral("saver"), saverLogin);
                                   savedDocument.appendChild(savedRoot);
                                   const QString savedPackage = savedDocument.toString(0);

                                   // The requester also receives the revision metadata so
                                   // its baseline stays aligned for subsequent mutations.
                                   connection->sendStringToClient(savedPackage);

                                   const QList<Connection *> partners = m_connectionList.value(projectID);
                                   for (Connection *partner : partners) {
                                       if (partner && partner != connection) {
                                           partner->sendNotification(385, collaboratorMessage,
                                                                     Notification::Info);
                                           partner->sendStringToClient(savedPackage);
                                       }
                                   }
                               }
                           } else {
                               connection->sendNotification(381, QObject::tr("Error saving project %1").arg(projectID),
                                                            Notification::Error);
                           }
                       } else {
                           connection->sendNotification(381, QObject::tr("Project %1 doesn't exist!").arg(projectID),
                                                        Notification::Error);
                       }
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_save package";
                       #endif
                       connection->sendNotification(381, QObject::tr("Invalid project save request"),
                                                    Notification::Error);
                   }
               } else {
                   connection->sendNotification(381, QObject::tr("Insufficient permissions to save project"),
                                                Notification::Error);
               }
    } else if (root == "project_image") {
               if (connection->student()->isEnabled()) {
                   ProjectImageParser parser;
                   if (parser.parse(package)) {
                       createImage(connection, parser.frame(), parser.scene(), parser.title(), parser.topics(), parser.description());
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_image package";
                       #endif
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), Notification::Warning);
               }
    } else if (root == "project_video") {
               if (connection->student()->isEnabled()) {
                   ProjectVideoParser parser;
                   if (parser.parse(package)) {
                       createVideo(connection, parser.title(), parser.topics(), parser.description(), parser.fps(), parser.scenes());
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_video package";
                       #endif
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), Notification::Warning);
               }
    } else if (root == "project_storyboard") {
               if (connection->student()->isEnabled()) {
                   qDebug() << "[ProjectManager::handlePackage()] - Exporting storyboard as work!";
                   ProjectStoryboardPostParser parser;
                   if (parser.parse(package)) {
                       createStoryboard(connection, parser.sceneIndex());
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_storyboard package";
                       #endif
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), Notification::Warning);
               }
    } else if (root == "project_storyboard_update") {
               if (connection->student()->isEnabled()) {
                   qDebug() << "[ProjectManager::handlePackage()] - Updating storyboard data in the project!";
                   ProjectStoryboardParser parser(package);

                   if (parser.checksum()) {
                       if ((parser.sceneIndex() >= 0) && (parser.storyboardXml().length() > 0)) {
                           updateStoryboard(connection, parser.sceneIndex(), parser.storyboardXml());
                           QDomDocument request = parser.request();
                           sendToProjectMembers(connection, request);
                       } else {
                           #ifdef TUP_DEBUG
                                  qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_storyboard package";
                           #endif
                       }
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_storyboard package";
                       #endif
                   }

                   /*
                    ProjectStoryboardParser parser;
                   if (parser.parse(package)) {
                       QDomDocument request;
                       request.setContent(package);
                       sendToProjectMembers(connection, request);
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "ProjectManager::handlePackage() - [ Fatal Error ] - Can't parse project_storyboard package";
                       #endif
                   }
                   */
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), Notification::Warning);
               }
    } else {
        #ifdef TUP_DEBUG
               QString ip = connection->client()->peerAddress().toString(); 
               Logger::self()->info(QObject::tr("Malformed package coming from -> %1").arg(ip));
               qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Malformed package coming from -> " << ip;
               qWarning() << package;
        #endif
    }
}

void ProjectManager::closeProject(const QString &projectID)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::closeProject()]";
    #endif

    // saveProject(projectID, true);
    delete m_openedProjects.take(projectID);
    delete m_commandResultRegistries.take(projectID);
    m_connectionList.remove(projectID);

    Logger::self()->info(QObject::tr("Project \"%1\" has been closed").arg(projectID));
    #ifdef TUP_DEBUG
           qWarning() << "[ProjectManager::closeProject()] - Project " << projectID << " has been closed";
    #endif
}

bool ProjectManager::saveProject(const QString &projectID, bool quiet)
{
    if (m_openedProjects.contains(projectID)) {
        bool ok = m_openedProjects.value(projectID)->save(quiet);
        if (ok)
            m_dbHandler->touchProjectUpdatedAt(projectID);
        return ok;
    }

    return false;
}

void ProjectManager::closeConnection(Connection *connection)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::closeConnection()]";
    #endif

    QString projectID = connection->data(Info::ProjectID).toString();
    if (projectID.isNull()) {
        #ifdef TUP_DEBUG
            qWarning() << "[ProjectManager::closeConnection()] - Warning: Connection pointer has NO project ID";
        #endif
        return;
    }

    if (connection->data(Info::ProjectIsOpen).toBool()) {
        m_connectionList[projectID].removeAll(connection);

        QString login = connection->student()->login();
        
        if (m_connectionList[projectID].isEmpty()) {
            closeProject(projectID);
        } else {
            // Notice msg("<b>" + login + "</b>" + " has left the project");
            Notice msg(login, 0);
            QList<Connection *> partners = m_connectionList[projectID];
            for (int i = 0; i < partners.size(); ++i)
                 partners.at(i)->sendStringToClient(msg.toString());
        }

        #ifdef TUP_DEBUG
            qWarning() << "[ProjectManager::closeConnection()] - Student " << login << " has logged off";
        #endif
    }
}

ProjectManager::ProjectCommandResult ProjectManager::handleProjectRequest(
    const QString &projectID,
    const QString &request,
    int studentId)
{
    ProjectCommandResult result;

    QDomDocument requestDocument;
    QString dependencyCommandId;
    QString clientId;
    QString commandType;
    qint64 baseRevision = 0;

    if (requestDocument.setContent(request)) {
        const QDomElement requestRoot = requestDocument.documentElement();

        result.commandId = requestRoot.attribute(
            QStringLiteral("command_id")).trimmed();
        dependencyCommandId = requestRoot.attribute(
            QStringLiteral("depends_on")).trimmed();
        clientId = requestRoot.attribute(
            QStringLiteral("client_id")).trimmed();
        commandType = requestRoot.attribute(
            QStringLiteral("command_type")).trimmed();

        bool baseRevisionOk = false;
        const qint64 parsedBaseRevision = requestRoot.attribute(
            QStringLiteral("base_revision")).toLongLong(&baseRevisionOk);
        if (baseRevisionOk && parsedBaseRevision >= 0)
            baseRevision = parsedBaseRevision;
    }

#ifdef TUP_DEBUG
    qDebug() << "[ProjectManager::handleProjectRequest()]";
    qWarning()
        << "[ProjectManager::handleProjectRequest()]"
        << "Looking for projectID:" << projectID;
    qWarning()
        << "[ProjectManager::handleProjectRequest()]"
        << "Opened projects:" << m_openedProjects.keys();
#endif

    const int dbProjectId = m_dbHandler->getProjectIdFromFilename(projectID);

    const auto attachRejectedConvertRestorePayload =
        [&](ProjectCommandResult *candidate) {
            if (!candidate
                    || candidate->status != ProjectCommandResult::Rejected
                    || !candidate->errorCode.startsWith(
                        QStringLiteral("conversion_restore_"))) {
                return;
            }

            TupRequestParser restoreParser;
            if (!restoreParser.parse(request))
                return;

            TupProjectResponse *restoreResponse = restoreParser.getResponse();
            if (!restoreResponse
                    || restoreResponse->getPart() != TupProjectRequest::Item
                    || restoreResponse->originalAction() != TupProjectRequest::Convert) {
                return;
            }

            NetProject *currentProject = m_openedProjects.value(projectID);
            if (!currentProject)
                return;

            const QString payload = currentConvertRepresentationPayload(
                static_cast<TupItemResponse *>(restoreResponse), currentProject);
            if (payload.isEmpty())
                return;

            candidate->hasAuthoritativePayload = true;
            candidate->eventPayload = payload;
        };

    CommandResultRegistry *registry =
        m_commandResultRegistries.value(projectID, nullptr);

    if (!result.commandId.isEmpty()) {
        if (registry && registry->contains(result.commandId)) {
            const CommandResultRegistry::StoredResult stored =
                registry->result(result.commandId);

            result.status =
                static_cast<ProjectCommandResult::Status>(stored.status);
            result.errorCode = stored.errorCode;
            result.message = stored.message;
            result.duplicate = true;

            // The in-memory registry intentionally stores only the terminal
            // result. Recover authoritative commit metadata from SQLite so a
            // duplicate/retried command_result can still advance the sender's
            // revision without replaying the mutation.
            if (stored.status == CommandResultRegistry::Committed
                    && dbProjectId > 0) {
                const DatabaseHandler::ProjectCommandRecord durable =
                    m_dbHandler->getProjectCommand(dbProjectId, result.commandId);
                if (durable.found
                        && durable.status == QStringLiteral("committed")) {
                    result.committedRevision = durable.committedRevision;
                    const DatabaseHandler::ProjectEventRecord event =
                        m_dbHandler->getProjectEventByCommand(
                            dbProjectId, result.commandId);
                    if (!event.commandId.isEmpty()) {
                        result.eventId = event.eventUuid;
                        result.eventType = event.eventType;
                        result.eventPayload = event.payload;
                        result.hasAuthoritativePayload =
                            !event.payload.isEmpty() && event.payload != request;
                    }
                }
            }

            attachRejectedConvertRestorePayload(&result);
#ifdef TUP_DEBUG
            qWarning()
                << "[ProjectManager::handleProjectRequest()]"
                << "Duplicate command found in memory:" << result.commandId;
#endif
            return result;
        }

        if (dbProjectId > 0) {
            const DatabaseHandler::ProjectCommandRecord stored =
                m_dbHandler->getProjectCommand(dbProjectId, result.commandId);

            if (stored.found) {
                if (stored.status == QStringLiteral("committed")) {
                    result.status = ProjectCommandResult::Committed;
                    result.errorCode = stored.errorCode;
                    result.message = stored.message;
                    result.committedRevision = stored.committedRevision;
                    const DatabaseHandler::ProjectEventRecord event =
                        m_dbHandler->getProjectEventByCommand(
                            dbProjectId, result.commandId);
                    if (!event.commandId.isEmpty()) {
                        result.eventId = event.eventUuid;
                        result.eventType = event.eventType;
                        result.eventPayload = event.payload;
                        result.hasAuthoritativePayload =
                            !event.payload.isEmpty() && event.payload != request;
                    }
                    result.duplicate = true;
                } else if (stored.status == QStringLiteral("rejected")) {
                    result.status = ProjectCommandResult::Rejected;
                    result.errorCode = stored.errorCode;
                    result.message = stored.message;
                    result.duplicate = true;
                } else if (stored.status == QStringLiteral("failed")) {
                    result.status = ProjectCommandResult::Failed;
                    result.errorCode = stored.errorCode;
                    result.message = stored.message;
                    result.duplicate = true;
                } else {
                    // A crash may have happened after the command changed the
                    // snapshot but before SQLite reached a terminal state.
                    // Re-executing here could apply a non-idempotent mutation twice.
                    result.status = ProjectCommandResult::Failed;
                    result.errorCode = QStringLiteral("command_recovery_required");
                    result.message = QObject::tr(
                        "The command has an unfinished server record and cannot be safely replayed.");
                    result.duplicate = true;
                }

                if (registry) {
                    registry->store(
                        result.commandId,
                        static_cast<CommandResultRegistry::Status>(result.status),
                        result.errorCode,
                        result.message);
                }

                attachRejectedConvertRestorePayload(&result);
#ifdef TUP_DEBUG
                qWarning()
                    << "[ProjectManager::handleProjectRequest()]"
                    << "Command found in durable registry:" << result.commandId
                    << "Status:" << stored.status
                    << "Revision:" << stored.committedRevision;
#endif
                return result;
            }
        }
    }

    TupRequestParser parser;

    if (!parser.parse(request)) {
        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("invalid_request_xml");
        result.message = QObject::tr("The project request could not be parsed.");

#ifdef TUP_DEBUG
        qWarning()
            << "[ProjectManager::handleProjectRequest()]"
            << result.message;
#endif
        if (registry && !result.commandId.isEmpty()) {
            registry->store(
                result.commandId,
                static_cast<CommandResultRegistry::Status>(result.status),
                result.errorCode,
                result.message);
        }
        return result;
    }

    TupProjectResponse *response = parser.getResponse();

    if (!response) {
        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("missing_response");
        result.message = QObject::tr(
            "The project request did not produce a response object.");

        qWarning()
            << "[ProjectManager::handleProjectRequest()]"
            << result.message;

        if (registry && !result.commandId.isEmpty()) {
            registry->store(
                result.commandId,
                static_cast<CommandResultRegistry::Status>(result.status),
                result.errorCode,
                result.message);
        }
        return result;
    }

    if (result.commandId.isEmpty())
        result.commandId = response->getCommandId();

    if (result.commandId.isEmpty()) {
        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("missing_command_id");
        result.message = QObject::tr(
            "The project request does not contain a command ID.");

        qWarning()
            << "[ProjectManager::handleProjectRequest()]"
            << result.message;

        delete response;
        return result;
    }

    NetProject *project = m_openedProjects.value(projectID);

    if (!project) {
        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("project_not_found");
        result.message = QObject::tr(
            "The requested project is not open on the server.");

        delete response;
        return result;
    }

    if (dbProjectId <= 0) {
        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("project_database_record_not_found");
        result.message = QObject::tr(
            "The project database record could not be resolved.");

        delete response;
        return result;
    }

#ifdef TUP_DEBUG
    qWarning()
        << "[ProjectManager::handleProjectRequest()]"
        << "Processing command:" << result.commandId
        << "DB project:" << dbProjectId
        << "Action:" << response->getAction()
        << "Part:" << response->getPart();
#endif

    if (!dependencyCommandId.isEmpty()) {
        if (dependencyCommandId == result.commandId) {
            result.status = ProjectCommandResult::Rejected;
            result.errorCode = QStringLiteral("self_dependency");
            result.message = QObject::tr(
                "A project command cannot depend on itself.");
        } else {
            bool dependencyFound = false;
            bool dependencyCommitted = false;

            if (registry && registry->contains(dependencyCommandId)) {
                const CommandResultRegistry::StoredResult dependencyResult =
                    registry->result(dependencyCommandId);
                dependencyFound = true;
                dependencyCommitted =
                    dependencyResult.status == CommandResultRegistry::Committed;
            } else {
                const DatabaseHandler::ProjectCommandRecord dependencyRecord =
                    m_dbHandler->getProjectCommand(dbProjectId, dependencyCommandId);
                if (dependencyRecord.found) {
                    dependencyFound = true;
                    dependencyCommitted =
                        dependencyRecord.status == QStringLiteral("committed");

                    if (registry && (dependencyRecord.status == QStringLiteral("committed")
                                     || dependencyRecord.status == QStringLiteral("rejected")
                                     || dependencyRecord.status == QStringLiteral("failed"))) {
                        CommandResultRegistry::Status cachedStatus =
                            CommandResultRegistry::Failed;
                        if (dependencyRecord.status == QStringLiteral("committed"))
                            cachedStatus = CommandResultRegistry::Committed;
                        else if (dependencyRecord.status == QStringLiteral("rejected"))
                            cachedStatus = CommandResultRegistry::Rejected;

                        registry->store(
                            dependencyCommandId,
                            cachedStatus,
                            dependencyRecord.errorCode,
                            dependencyRecord.message);
                    }
                }
            }

            if (!dependencyFound) {
                result.status = ProjectCommandResult::Rejected;
                result.errorCode = QStringLiteral("dependency_not_found");
                result.message = QObject::tr(
                    "The prerequisite command is unknown to this project.");
            } else if (!dependencyCommitted) {
                result.status = ProjectCommandResult::Rejected;
                result.errorCode = QStringLiteral("dependency_not_committed");
                result.message = QObject::tr(
                    "The prerequisite command was not committed.");
            }
        }

        if (result.status == ProjectCommandResult::Rejected) {
            // Rejected dependency checks are terminal but the command was never
            // executed, so persist their result for retry/idempotency handling.
            const QByteArray requestHash = QCryptographicHash::hash(
                request.toUtf8(), QCryptographicHash::Sha256).toHex();

            if (m_dbHandler->insertProjectCommand(
                    dbProjectId,
                    result.commandId,
                    studentId,
                    clientId,
                    commandType,
                    baseRevision,
                    dependencyCommandId,
                    QString::fromLatin1(requestHash),
                    QStringLiteral("processing"))) {
                m_dbHandler->updateProjectCommandResult(
                    dbProjectId,
                    result.commandId,
                    QStringLiteral("rejected"),
                    result.errorCode,
                    result.message);
            }

            if (registry) {
                registry->store(
                    result.commandId,
                    CommandResultRegistry::Rejected,
                    result.errorCode,
                    result.message);
            }

            delete response;
            return result;
        }
    }

    const QByteArray requestHash = QCryptographicHash::hash(
        request.toUtf8(), QCryptographicHash::Sha256).toHex();

    if (!m_dbHandler->insertProjectCommand(
            dbProjectId,
            result.commandId,
            studentId,
            clientId,
            commandType,
            baseRevision,
            dependencyCommandId,
            QString::fromLatin1(requestHash),
            QStringLiteral("processing"))) {
        // A concurrent/retried request may have inserted the same command
        // between our first lookup and this insert. Re-read before failing.
        const DatabaseHandler::ProjectCommandRecord stored =
            m_dbHandler->getProjectCommand(dbProjectId, result.commandId);

        if (stored.found) {
            if (stored.status == QStringLiteral("committed")) {
                result.status = ProjectCommandResult::Committed;
                result.committedRevision = stored.committedRevision;
                const DatabaseHandler::ProjectEventRecord event =
                    m_dbHandler->getProjectEventByCommand(
                        dbProjectId, result.commandId);
                if (!event.commandId.isEmpty()) {
                    result.eventId = event.eventUuid;
                    result.eventType = event.eventType;
                    result.eventPayload = event.payload;
                    result.hasAuthoritativePayload =
                        !event.payload.isEmpty() && event.payload != request;
                }
            } else if (stored.status == QStringLiteral("rejected"))
                result.status = ProjectCommandResult::Rejected;
            else
                result.status = ProjectCommandResult::Failed;

            result.errorCode = stored.errorCode;
            result.message = stored.message;
            result.duplicate = true;
        } else {
            result.status = ProjectCommandResult::Failed;
            result.errorCode = QStringLiteral("command_registry_write_failed");
            result.message = QObject::tr(
                "The command could not be registered in persistent storage.");
        }

        attachRejectedConvertRestorePayload(&result);
        delete response;
        return result;
    }

    if (response->getPart() == TupProjectRequest::Item
            && response->originalAction() == TupProjectRequest::Convert) {
        TupItemResponse *itemResponse = static_cast<TupItemResponse *>(response);
        QString restoreError;
        QString authoritativeCurrentPayload;
        if (prepareAuthoritativeConvertRestore(
                itemResponse, m_dbHandler, dbProjectId, studentId, project,
                &restoreError, &authoritativeCurrentPayload)
                && !restoreError.isEmpty()) {
            result.status = ProjectCommandResult::Rejected;
            result.errorCode = restoreError;
            result.message = restoreError == QStringLiteral("conversion_restore_not_owner")
                ? QObject::tr("Only the author of the original conversion can undo or redo it.")
                : (restoreError == QStringLiteral("conversion_restore_conflict")
                    ? QObject::tr("The object changed after the conversion, so this undo or redo is no longer safe.")
                    : QObject::tr(
                        "The conversion restore request could not be resolved authoritatively."));
            result.hasAuthoritativePayload = !authoritativeCurrentPayload.isEmpty();
            result.eventPayload = authoritativeCurrentPayload;

            m_dbHandler->updateProjectCommandResult(
                dbProjectId,
                result.commandId,
                QStringLiteral("rejected"),
                result.errorCode,
                result.message);

            if (registry) {
                registry->store(
                    result.commandId,
                    CommandResultRegistry::Rejected,
                    result.errorCode,
                    result.message);
            }

            delete response;
            return result;
        }
    }

    bool authoritativeEditNodesRestore = false;
    if (response->getPart() == TupProjectRequest::Item
            && response->originalAction() == TupProjectRequest::EditNodes) {
        TupItemResponse *itemResponse = static_cast<TupItemResponse *>(response);
        QString restoreError;
        authoritativeEditNodesRestore = prepareAuthoritativeEditNodesRestore(
            itemResponse, m_dbHandler, dbProjectId, studentId, project,
            &restoreError);
        if (authoritativeEditNodesRestore && !restoreError.isEmpty()) {
            result.status = ProjectCommandResult::Rejected;
            result.errorCode = restoreError;
            result.message = restoreError == QStringLiteral("edit_nodes_restore_not_owner")
                ? QObject::tr("Only the author of the original node edit can undo or redo it.")
                : (restoreError == QStringLiteral("edit_nodes_restore_conflict")
                    ? QObject::tr("The path changed after this node edit, so this undo or redo is no longer safe.")
                    : QObject::tr(
                        "The node-edit restore request could not be resolved authoritatively."));

            m_dbHandler->updateProjectCommandResult(
                dbProjectId,
                result.commandId,
                QStringLiteral("rejected"),
                result.errorCode,
                result.message);

            if (registry) {
                registry->store(
                    result.commandId,
                    CommandResultRegistry::Rejected,
                    result.errorCode,
                    result.message);
            }

            delete response;
            return result;
        }
    }

    if (response->getPart() == TupProjectRequest::Item
            && response->originalAction() == TupProjectRequest::Transform) {
        TupItemResponse *itemResponse = static_cast<TupItemResponse *>(response);
        const bool authoritativeTransformCandidate =
            itemResponse->spaceMode() == TupProject::FRAMES_MODE
            && itemResponse->getItemType() != TupLibraryObject::Svg
            && !itemResponse->getObjectId().trimmed().isEmpty();
        if (authoritativeTransformCandidate) {
            QString restoreError;
        const bool isRestore = prepareAuthoritativeTransformRestore(
            itemResponse, m_dbHandler, dbProjectId, studentId, project, &restoreError);
        if (isRestore && !restoreError.isEmpty()) {
            result.status = ProjectCommandResult::Rejected;
            result.errorCode = restoreError;
            result.message = restoreError == QStringLiteral("transform_restore_not_owner")
                ? QObject::tr("Only the author of the original transform can undo or redo it.")
                : (restoreError == QStringLiteral("transform_restore_conflict")
                    ? QObject::tr("The object changed after the transform, so this undo or redo is no longer safe.")
                    : QObject::tr("The transform restore request could not be resolved authoritatively."));
            m_dbHandler->updateProjectCommandResult(
                dbProjectId, result.commandId, QStringLiteral("rejected"),
                result.errorCode, result.message);
            if (registry) {
                registry->store(result.commandId, CommandResultRegistry::Rejected,
                                result.errorCode, result.message);
            }
            delete response;
            return result;
        }

        if (!isRestore) {
            const QString sourceProperties = currentTransformProperties(itemResponse, project);
            if (sourceProperties.isEmpty()) {
                result.status = ProjectCommandResult::Rejected;
                result.errorCode = QStringLiteral("transform_source_snapshot_failed");
                result.message = QObject::tr(
                    "The transform source state could not be captured authoritatively.");
                m_dbHandler->updateProjectCommandResult(
                    dbProjectId, result.commandId, QStringLiteral("rejected"),
                    result.errorCode, result.message);
                if (registry) {
                    registry->store(result.commandId, CommandResultRegistry::Rejected,
                                    result.errorCode, result.message);
                }
                delete response;
                return result;
            }
            response->setData(sourceProperties.toUtf8());
        }
        }
    }

    TupCommandExecutor *commandExecutor =
        new TupCommandExecutor(project);

    TupProjectCommand command(commandExecutor, response);
    command.redo();

    project->resetTimer();

    if (!command.succeeded()) {
        result.status = ProjectCommandResult::Rejected;
        result.errorCode = command.errorCode();

        if (result.errorCode.isEmpty())
            result.errorCode = QStringLiteral("execution_failed");

        result.message = QObject::tr(
            "The project command was rejected by the server.");

        if (!m_dbHandler->updateProjectCommandResult(
                dbProjectId,
                result.commandId,
                QStringLiteral("rejected"),
                result.errorCode,
                result.message)) {
            qWarning()
                << "[ProjectManager::handleProjectRequest()]"
                << "Unable to persist rejected command result:"
                << result.commandId;
        }

        if (registry) {
            registry->store(
                result.commandId,
                CommandResultRegistry::Rejected,
                result.errorCode,
                result.message);
        }

        delete commandExecutor;
        return result;
    }

    result.eventType = command.eventType();
    result.hasAuthoritativePayload = command.hasAuthoritativeEventPayload();
    result.eventPayload = result.hasAuthoritativePayload
        ? command.authoritativeEventPayload()
        : request;
    if (authoritativeEditNodesRestore) {
        const TupProjectRequest authoritativeRequest =
            TupRequestBuilder::fromResponse(response, true);
        result.eventPayload = authoritativeRequest.getXml();
        result.hasAuthoritativePayload = !result.eventPayload.isEmpty();
    }
    result.eventId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    if (result.eventType.isEmpty())
        result.eventType = QStringLiteral("project.command-committed");

    // Domain execution succeeded, but the command is not committed yet.
    // Persist it first as a revision-specific pending snapshot. SQLite then
    // atomically commits the command/event/revision metadata that identifies
    // that exact snapshot. Only after that commit do we promote the package to
    // the normal authoritative .tup path.
    const DatabaseHandler::ProjectRevisionInfo revisionInfo =
        m_dbHandler->getProjectRevisionInfo(dbProjectId);
    if (!revisionInfo.found
            || revisionInfo.currentRevision != revisionInfo.snapshotRevision) {
        command.undo();
        delete commandExecutor;

        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("revision_metadata_inconsistent");
        result.message = QObject::tr(
            "The server project revision metadata is inconsistent.");

        m_dbHandler->updateProjectCommandResult(
            dbProjectId, result.commandId, QStringLiteral("failed"),
            result.errorCode, result.message);
        if (registry) {
            registry->store(result.commandId, CommandResultRegistry::Failed,
                            result.errorCode, result.message);
        }
        return result;
    }

    const qint64 stagedRevision = revisionInfo.currentRevision + 1;
    QString snapshotChecksum;
    FileManager snapshotManager;
    if (!snapshotManager.savePendingSnapshot(
            projectID, project, project->owner(), stagedRevision, &snapshotChecksum)) {
        command.undo();
        const bool rollbackOk = command.succeeded();
        delete commandExecutor;

        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("persistence_failed");
        result.message = QObject::tr(
            "The project command executed but its durable snapshot could not be persisted.");

        m_dbHandler->updateProjectCommandResult(
            dbProjectId,
            result.commandId,
            QStringLiteral("failed"),
            result.errorCode,
            result.message);

        if (registry) {
            registry->store(
                result.commandId,
                CommandResultRegistry::Failed,
                result.errorCode,
                result.message);
        }

        qCritical()
            << "[ProjectManager::handleProjectRequest()]"
            << "Pending snapshot persistence failed after domain mutation."
            << "Command:" << result.commandId
            << "Rollback succeeded:" << rollbackOk;
        return result;
    }

    qint64 committedRevision = -1;
    if (!m_dbHandler->finalizeCommittedProjectCommand(
            dbProjectId,
            result.commandId,
            stagedRevision,
            &committedRevision,
            result.eventId,
            result.eventType,
            result.eventPayload,
            snapshotChecksum)) {
        snapshotManager.discardPendingSnapshot(projectID, project->owner(), stagedRevision);
        command.undo();
        const bool rollbackOk = command.succeeded();
        delete commandExecutor;

        result.status = ProjectCommandResult::Failed;
        result.errorCode = QStringLiteral("commit_metadata_failed");
        result.message = QObject::tr(
            "The project snapshot was staged, but the command/event metadata could not be committed.");

        // Best effort: convert the unfinished durable record to a terminal
        // failure so a retry is not silently executed again.
        m_dbHandler->updateProjectCommandResult(
            dbProjectId,
            result.commandId,
            QStringLiteral("failed"),
            result.errorCode,
            result.message);

        if (registry) {
            registry->store(
                result.commandId,
                CommandResultRegistry::Failed,
                result.errorCode,
                result.message);
        }

        qCritical()
            << "[ProjectManager::handleProjectRequest()]"
            << "Pending snapshot staged but SQLite commit metadata failed."
            << "Command:" << result.commandId
            << "Rollback succeeded:" << rollbackOk;
        return result;
    }

    if (!snapshotManager.promotePendingSnapshot(
            projectID, project->owner(), committedRevision, snapshotChecksum)) {
        // The command is still durably committed: SQLite identifies the exact
        // revision-specific pending package. Startup reconciliation will
        // promote it before any client is allowed to open the project.
        qCritical()
            << "[ProjectManager::handleProjectRequest()]"
            << "Committed snapshot promotion deferred to startup recovery."
            << "Command:" << result.commandId
            << "Revision:" << committedRevision;
    }

    delete commandExecutor;

    result.status = ProjectCommandResult::Committed;
    result.errorCode.clear();
    result.message.clear();
    result.committedRevision = committedRevision;

    if (registry) {
        registry->store(
            result.commandId,
            CommandResultRegistry::Committed,
            result.errorCode,
            result.message);
    }

#ifdef TUP_DEBUG
    qWarning()
        << "[ProjectManager::handleProjectRequest()]"
        << "Command durably committed:" << result.commandId
        << "Revision:" << committedRevision
        << "Event:" << result.eventId
        << "Type:" << result.eventType
        << "Registry size:" << (registry ? registry->count() : 0);
#endif

    return result;
}

void ProjectManager::sendCommandResult(
    Connection *connection,
    const ProjectCommandResult &result)
{
    if (!connection) {
        qWarning()
            << "[ProjectManager::sendCommandResult()]"
            << "Connection is null.";
        return;
    }

    CommandResult::Status packageStatus = CommandResult::Failed;

    switch (result.status) {
        case ProjectCommandResult::Committed:
            packageStatus = CommandResult::Committed;
            break;

        case ProjectCommandResult::Rejected:
            packageStatus = CommandResult::Rejected;
            break;

        case ProjectCommandResult::Failed:
            packageStatus = CommandResult::Failed;
            break;
    }

    CommandResult package(
        result.commandId,
        packageStatus,
        result.errorCode,
        result.message);

#ifdef TUP_DEBUG
    qWarning()
        << "[ProjectManager::sendCommandResult()]"
        << "Sending result for command:" << result.commandId
        << "Status:" << static_cast<int>(result.status)
        << "Error:" << result.errorCode;
#endif

    QString xml = package.toString(0);

    if ((result.isCommitted() && result.committedRevision > 0)
            || (result.hasAuthoritativePayload && !result.eventPayload.isEmpty())) {
        QDomDocument document;
        if (document.setContent(xml)) {
            QDomElement root = document.documentElement();
            if (!root.isNull()
                    && root.tagName() == QStringLiteral("command_result")) {
                if (result.isCommitted() && result.committedRevision > 0) {
                    root.setAttribute(
                        QStringLiteral("committed_revision"),
                        result.committedRevision);
                    root.setAttribute(QStringLiteral("event_index"), 0);
                    if (!result.eventType.isEmpty())
                        root.setAttribute(QStringLiteral("event_type"), result.eventType);
                }

                if (result.hasAuthoritativePayload
                        && !result.eventPayload.isEmpty()) {
                    QDomElement payload = document.createElement(
                        QStringLiteral("authoritative_payload"));
                    payload.appendChild(
                        document.createTextNode(result.eventPayload));
                    root.appendChild(payload);
                }

                xml = document.toString(0);
            }
        }
    }

    connection->sendStringToClient(xml);
}

void ProjectManager::listStudentProjects(Connection *connection)
{
    ProjectList list;
    int uid = connection->student()->uid();
    QString login = connection->student()->login();

    foreach (DatabaseHandler::ProjectInfo info, m_dbHandler->studentProjects(uid, login))
             list.addProject(ProjectList::Work, info.file, info.title, info.owner, info.description, info.date);

    foreach (DatabaseHandler::ProjectInfo info, m_dbHandler->partnerProjects(uid))
             list.addProject(ProjectList::Contribution, info.file, info.title, info.owner, info.description, info.date);

    connection->sendStringToClient(list.toString());
}

void ProjectManager::sendStoredProjectEvent(
    Connection *connection,
    const QString &projectID,
    const DatabaseHandler::ProjectEventRecord &event)
{
    if (!connection || projectID.isEmpty() || event.eventUuid.isEmpty()
            || event.commandId.isEmpty() || event.revision <= 0
            || event.eventIndex < 0 || event.eventType.isEmpty()
            || event.payload.isEmpty()) {
        qWarning() << "[ProjectManager::sendStoredProjectEvent()] Incomplete stored event.";
        return;
    }

    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("project_event"));
    root.setAttribute(QStringLiteral("version"), QStringLiteral("1"));
    root.setAttribute(QStringLiteral("event_id"), event.eventUuid);
    root.setAttribute(QStringLiteral("caused_by"), event.commandId);
    root.setAttribute(QStringLiteral("project_id"), projectID);
    root.setAttribute(QStringLiteral("revision"), event.revision);
    root.setAttribute(QStringLiteral("event_index"), event.eventIndex);
    root.setAttribute(QStringLiteral("event_type"), event.eventType);
    document.appendChild(root);

    QDomElement payload = document.createElement(QStringLiteral("payload"));
    payload.appendChild(document.createTextNode(event.payload));
    root.appendChild(payload);

    connection->sendStringToClient(document.toString(0));
}

void ProjectManager::sendProjectSyncResponse(
    Connection *connection,
    const QString &projectID,
    const QString &mode,
    qint64 fromRevision,
    qint64 toRevision,
    qint64 savedRevision,
    int eventCount)
{
    if (!connection)
        return;

    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("project_sync_response"));
    root.setAttribute(QStringLiteral("version"), QStringLiteral("1"));
    root.setAttribute(QStringLiteral("project_id"), projectID);
    root.setAttribute(QStringLiteral("mode"), mode);
    root.setAttribute(QStringLiteral("from_revision"), fromRevision);
    root.setAttribute(QStringLiteral("to_revision"), toRevision);
    root.setAttribute(QStringLiteral("saved_revision"), savedRevision);
    root.setAttribute(QStringLiteral("event_count"), eventCount);
    document.appendChild(root);
    connection->sendStringToClient(document.toString(0));
}

void ProjectManager::handleProjectSyncRequest(Connection *connection, const QString &package)
{
    if (!connection || !connection->student() || !connection->student()->isEnabled())
        return;

    QDomDocument document;
    if (!document.setContent(package)) {
        qWarning() << "[ProjectManager::handleProjectSyncRequest()] Invalid XML.";
        return;
    }

    const QDomElement root = document.documentElement();
    if (root.tagName() != QStringLiteral("project_sync_request"))
        return;

    const QString projectID = root.attribute(QStringLiteral("project_id")).trimmed();
    const QString owner = root.attribute(QStringLiteral("owner")).trimmed();

    bool revisionOk = false;
    const qint64 lastRevision = root.attribute(QStringLiteral("last_revision")).toLongLong(&revisionOk);
    bool eventIndexOk = false;
    const int lastEventIndex = root.attribute(QStringLiteral("last_event_index")).toInt(&eventIndexOk);
    const bool forceSnapshot = root.attribute(QStringLiteral("force_snapshot")) == QStringLiteral("1");

    if (projectID.isEmpty() || owner.isEmpty() || !revisionOk || lastRevision < -1
            || !eventIndexOk || lastEventIndex < -1) {
        qWarning() << "[ProjectManager::handleProjectSyncRequest()] Incomplete sync request.";
        return;
    }

    const QString ownerID = m_dbHandler->studentID(owner);
    const QString dbProjectKey = m_dbHandler->exists(projectID, ownerID);
    if (dbProjectKey == QStringLiteral("-1")
            || !m_dbHandler->accessIsConfirmed(dbProjectKey, connection->student()->uid())) {
        connection->sendNotification(360, QObject::tr("Insufficient Permissions"), Notification::Error);
        return;
    }

    const int dbProjectId = dbProjectKey.toInt();
    const DatabaseHandler::ProjectRevisionInfo revisionInfo =
        m_dbHandler->getProjectRevisionInfo(dbProjectId);
    if (!revisionInfo.found)
        return;

    // A client that is already at the authoritative revision needs no
    // snapshot, even if the server has just restarted and therefore has no
    // in-memory NetProject yet. Reload/register the project server-side
    // without transmitting a redundant snapshot, then acknowledge a
    // zero-event synchronization.
    const bool clientAlreadyCurrent = !forceSnapshot
        && lastRevision == revisionInfo.currentRevision
        && ((revisionInfo.currentRevision == 0 && lastEventIndex <= 0)
            || (revisionInfo.currentRevision > 0 && lastEventIndex >= 0));

    if (clientAlreadyCurrent) {
        if (m_openedProjects.contains(projectID)) {
            registerProject(connection, ownerID, projectID,
                            m_openedProjects.value(projectID), false);
        } else {
            openProject(projectID, owner, connection, false);
        }

        if (connection->data(Info::ProjectIsOpen).toBool()) {
#ifdef TUP_DEBUG
            qWarning() << "[ProjectManager::handleProjectSyncRequest()] Client already current. Project:"
                       << projectID << "Revision:" << revisionInfo.currentRevision
                       << "Saved revision:" << revisionInfo.savedRevision;
#endif
            sendProjectSyncResponse(connection, projectID, QStringLiteral("events"),
                                    lastRevision, revisionInfo.currentRevision,
                                    revisionInfo.savedRevision, 0);
        }
        return;
    }

    const int catchUpLimit = 500;
    bool useEvents = !forceSnapshot
        && lastRevision >= 0
        && lastRevision <= revisionInfo.currentRevision
        && m_openedProjects.contains(projectID);

    QList<DatabaseHandler::ProjectEventRecord> events;
    if (useEvents) {
        events = m_dbHandler->getProjectEventsAfter(
            dbProjectId, lastRevision, lastEventIndex, catchUpLimit + 1);

        if (events.size() > catchUpLimit) {
            useEvents = false;
        } else {
            qint64 expectedRevision = lastRevision;
            int expectedEventIndex = lastEventIndex;

            for (const DatabaseHandler::ProjectEventRecord &event : events) {
                const bool nextSameRevision =
                    expectedEventIndex >= 0
                    && event.revision == expectedRevision
                    && event.eventIndex == expectedEventIndex + 1;
                const bool nextRevision =
                    event.revision == expectedRevision + 1
                    && event.eventIndex == 0;

                if (!nextSameRevision && !nextRevision) {
                    useEvents = false;
                    break;
                }

                expectedRevision = event.revision;
                expectedEventIndex = event.eventIndex;
            }

            if (useEvents && revisionInfo.currentRevision > lastRevision) {
                if (events.isEmpty()
                        || events.last().revision != revisionInfo.currentRevision) {
                    useEvents = false;
                }
            }
        }
    }

    if (useEvents) {
        NetProject *project = m_openedProjects.value(projectID, nullptr);
        if (!project)
            useEvents = false;
        else {
            registerProject(connection, ownerID, projectID, project, false);

#ifdef TUP_DEBUG
            qWarning() << "[ProjectManager::handleProjectSyncRequest()] Event catch-up. Project:"
                       << projectID << "From:" << lastRevision << lastEventIndex
                       << "To:" << revisionInfo.currentRevision
                       << "Events:" << events.size();
#endif

            for (const DatabaseHandler::ProjectEventRecord &event : events)
                sendStoredProjectEvent(connection, projectID, event);

            sendProjectSyncResponse(connection, projectID, QStringLiteral("events"),
                                    lastRevision, revisionInfo.currentRevision,
                                    revisionInfo.savedRevision, events.size());
            return;
        }
    }

#ifdef TUP_DEBUG
    qWarning() << "[ProjectManager::handleProjectSyncRequest()] Snapshot fallback. Project:"
               << projectID << "Client revision:" << lastRevision
               << "Server revision:" << revisionInfo.currentRevision;
#endif

    if (m_openedProjects.contains(projectID)) {
        registerProject(connection, ownerID, projectID, m_openedProjects.value(projectID), true);
    } else {
        openProject(projectID, owner, connection);
    }

    if (connection->data(Info::ProjectIsOpen).toBool()) {
        sendProjectSyncResponse(connection, projectID, QStringLiteral("snapshot"),
                                lastRevision, revisionInfo.currentRevision,
                                revisionInfo.savedRevision, 0);

    }
}

void ProjectManager::sendProjectEventToProjectMembers(
    Connection *connection,
    const ProjectCommandResult &result)
{
    if (!connection) {
        qWarning()
            << "[ProjectManager::sendProjectEventToProjectMembers()]"
            << "Connection is null.";
        return;
    }

    if (!result.isCommitted()
            || result.eventId.isEmpty()
            || result.eventType.isEmpty()
            || result.committedRevision <= 0
            || result.eventPayload.isEmpty()) {
        qWarning()
            << "[ProjectManager::sendProjectEventToProjectMembers()]"
            << "Cannot broadcast an incomplete project event."
            << "Command:" << result.commandId
            << "Event:" << result.eventId
            << "Revision:" << result.committedRevision;
        return;
    }

    const QString projectID =
        connection->data(Info::ProjectID).toString();

    if (projectID.isEmpty()) {
        qWarning()
            << "[ProjectManager::sendProjectEventToProjectMembers()]"
            << "Project ID is undefined.";
        return;
    }

    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("project_event"));
    root.setAttribute(QStringLiteral("version"), QStringLiteral("1"));
    root.setAttribute(QStringLiteral("event_id"), result.eventId);
    root.setAttribute(QStringLiteral("caused_by"), result.commandId);
    root.setAttribute(QStringLiteral("project_id"), projectID);
    root.setAttribute(QStringLiteral("revision"), result.committedRevision);
    root.setAttribute(QStringLiteral("event_index"), 0);
    root.setAttribute(QStringLiteral("event_type"), result.eventType);
    document.appendChild(root);

    QDomElement payload = document.createElement(QStringLiteral("payload"));
    payload.appendChild(document.createTextNode(result.eventPayload));
    root.appendChild(payload);

#ifdef TUP_DEBUG
    qWarning()
        << "[ProjectManager::sendProjectEventToProjectMembers()]"
        << "Broadcasting authoritative event:"
        << result.eventId
        << "Command:" << result.commandId
        << "Type:" << result.eventType
        << "Revision:" << result.committedRevision
        << "Project:" << projectID;
#endif

    foreach (Connection *link, m_connectionList[projectID]) {
        if (!link || !link->student())
            continue;

        // The sender has already applied the command optimistically. Its
        // authoritative confirmation is command_result, so do not execute
        // the same mutation a second time on that client.
        if (link->student()->uid() == connection->student()->uid())
            continue;

        if (!link->student()->isEnabled())
            continue;

#ifdef TUP_DEBUG
        qWarning()
            << "[ProjectManager::sendProjectEventToProjectMembers()]"
            << "Sending event:" << result.eventId
            << "to:" << link->student()->login();
#endif

        link->sendStringToClient(document.toString(0));
    }
}

void ProjectManager::sendToProjectMembers(
    Connection *connection,
    QDomDocument &doc)
{
#ifdef TUP_DEBUG
    qDebug() << "[ProjectManager::sendToProjectMembers()]";
    qDebug()
        << "[ProjectManager::sendToProjectMembers()]"
        << "Sending request to clients...";
#endif

    if (!connection) {
        qWarning()
            << "[ProjectManager::sendToProjectMembers()]"
            << "Connection is null.";
        return;
    }

    QDomElement root = doc.documentElement();

    if (root.isNull()
            || root.tagName() != QStringLiteral("project_request")) {
        qWarning()
            << "[ProjectManager::sendToProjectMembers()]"
            << "Invalid project request document.";
        return;
    }

    const QString commandId =
        root.attribute(QStringLiteral("command_id"));

    if (commandId.isEmpty()) {
        qWarning()
            << "[ProjectManager::sendToProjectMembers()]"
            << "Cannot broadcast a request without a command ID.";
        return;
    }

    const QString projectID =
        connection->data(Info::ProjectID).toString();

    connection->signPackage(doc);

#ifdef TUP_DEBUG
    qWarning()
        << "[ProjectManager::sendToProjectMembers()]"
        << "Broadcasting command:" << commandId;
    qWarning()
        << "[ProjectManager::sendToProjectMembers()]"
        << "ProjectID:" << projectID;
    qWarning()
        << "[ProjectManager::sendToProjectMembers()]"
        << "Sender UID:" << connection->student()->uid()
        << "Login:" << connection->student()->login();
    qWarning()
        << "[ProjectManager::sendToProjectMembers()]"
        << "Total connections for project:"
        << m_connectionList[projectID].size();
#endif

    foreach (Connection *link, m_connectionList[projectID]) {
        if (!link || !link->student())
            continue;

#ifdef TUP_DEBUG
        qWarning()
            << "[ProjectManager::sendToProjectMembers()]"
            << "Checking link UID:" << link->student()->uid()
            << "Login:" << link->student()->login();
#endif

        if (link->student()->uid()
                == connection->student()->uid()) {
            continue;
        }

        if (!link->student()->isEnabled()) {
#ifdef TUP_DEBUG
            qWarning()
                << "[ProjectManager::sendToProjectMembers()]"
                << "Student NOT enabled:"
                << link->student()->login();
#endif
            continue;
        }

#ifdef TUP_DEBUG
        qWarning()
            << "[ProjectManager::sendToProjectMembers()]"
            << "Sending command:" << commandId
            << "to:" << link->student()->login();
#endif

        link->sendStringToClient(doc.toString(0));
    }
}

void ProjectManager::loadVideoPlugin()
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::loadVideoPlugin()]";
        qDebug() << "[ProjectManager::loadVideoPlugin()] - Loading video plugin from: " << PLUGINS_DIR;
    #endif

    QHash<QString, TupExportInterface *> m_plugins;
    QDir pluginDirectory = QDir(PLUGINS_DIR);

    bool found = false;

    foreach (QString fileName, pluginDirectory.entryList(QDir::Files)) {
        #ifdef Q_OS_WIN
            const bool isPlugin = (fileName.compare("tupiffmpegplugin.dll") == 0);
        #else
            const bool isPlugin = (fileName.compare("libtupiffmpegplugin.so") == 0);
        #endif
        if (isPlugin) {
            #ifdef TUP_DEBUG
                qDebug() << "[ProjectManager::loadVideoPlugin()] - Plugin was found! Loading...";
            #endif

            QPluginLoader loader(pluginDirectory.absoluteFilePath(fileName));
            TupExportPluginObject *plugin = qobject_cast<TupExportPluginObject*>(loader.instance());

            if (plugin) {
                m_exporter = qobject_cast<TupExportInterface *>(plugin);
                if (m_exporter) {
                    found = true;
                    #ifdef TUP_DEBUG
                        qWarning() << "[ProjectManager::loadVideoPlugin()] - Plugin for video exportation is loaded -> " << fileName;
                    #endif
                }
            }
        }
    }

    if (!found) {
        #ifdef TUP_DEBUG
               qDebug() << "[ProjectManager::loadVideoPlugin()] - Fatal Error: Plugin for video exportation was not found!";
        #endif
    }
}

/*
void ProjectManager::updateProcessStatus(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);

    if ((exitStatus == QProcess::NormalExit) && (m_videoUrl.length() > 0)) {
        QString msg = "ProjectManager::updateProcessStatus() - Returning";
        if (formatVersion == 0) {
            #ifdef TUP_DEBUG
                qDebug() << msg + " url -> " << m_videoUrl;
            #endif
        } else {
            #ifdef TUP_DEBUG
                qDebug() << msg + " video path -> " << m_videoUrl;
            #endif
        }

        // SQA: Code temporary disabled
        // postWork();
        m_connection->sendStringToClient(m_videoUrl);
    }
}

void ProjectManager::postFinished(QNetworkReply *reply)
{
   #ifdef TUP_DEBUG
       qDebug() << "ProjectManager::postFinished() - Tracing...";
   #endif

    QByteArray array = reply->readAll();
    QString answer(array);

    #ifdef TUP_DEBUG
        qDebug() << "ProjectManager::postFinished() - answer: ";
        qDebug() << answer;
    #endif

    networkManager->deleteLater();
}

void ProjectManager::postWork()
{
    #ifdef TUP_DEBUG
        qDebug() << "ProjectManager::postWork() - Tracing...";
    #endif

    QString path = kAppProp->repositoryDir() + "mobile/animations/" + m_videoFilename + ".mp4";
    if (QFile::exists(path)) {
        qDebug() << "ProjectManager::postWork() - Registering video work in the social network...";

        // action (a)
        // studentname (u)
        // filename (i)
        // type == "video" (p)
        // title (t)
        // topics (h)
        // description (d)
        // url: /?a=post&t=title&u=studentname&p=video&i=filename&h=topics&d=description

        QUrl serviceUrl = QUrl("http://dev.tupitube.co/api/index.php");
        QByteArray postData;
        postData.append("a=post&");
        postData.append("t=" + m_videoTitle + "&");
        postData.append("u=" + m_videoStudentname + "&");
        postData.append("p=video&");
        postData.append("i=" + m_videoFilename + "&");
        postData.append("h=" + m_videoTopics + "&");
        postData.append("d=" + m_videoDescription);

        QString parameters(postData);
        qDebug() << "ProjectManager::postWork() - Post parameters: " << parameters;

        networkManager = new QNetworkAccessManager(this);
        connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(postFinished(QNetworkReply*)));

        QNetworkRequest request(serviceUrl);
        request.setRawHeader("Student-Agent", BROWSER_FINGERPRINT.toLatin1());

        // request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QNetworkReply *reply = networkManager->post(request, postData);
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "ProjectManager::postWork() - [ Fatal Error ] - Video file doesn't exist -> " << path;
        #endif
    }
}

void ProjectManager::slotError(QNetworkReply::NetworkError error)
{
    switch (error) {
        case QNetworkReply::HostNotFoundError:
             { 
             #ifdef TUP_DEBUG
                 qDebug() << "ProjectManager::sloqDebug() - Network Error: Host not found";
             #endif
             }
        break;
        case QNetworkReply::TimeoutError:
             {
             #ifdef TUP_DEBUG
                 qDebug() << "ProjectManager::sloqDebug() - Network Error: Time out!";
             #endif
             }
        break;
        case QNetworkReply::ConnectionRefusedError:
             {
             #ifdef TUP_DEBUG
                 qDebug() << "ProjectManager::sloqDebug() - Network Error: Connection Refused!";
             #endif
            }
        break;
        case QNetworkReply::ContentNotFoundError:
            {
             #ifdef TUP_DEBUG
                 qDebug() << "ProjectManager::sloqDebug() - Network Error: Content not found!";
             #endif
            }
        break;
        case QNetworkReply::UnknownNetworkError:
        default:
            {
             #ifdef TUP_DEBUG
                 qDebug() << "ProjectManager::sloqDebug() - Network Error: Unknown Network error!";
             #endif
            }
        break;
    }
}
*/

bool ProjectManager::resizeVideo(const QString &code, const QString &input, const QSize &size)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::resizeVideo()] - Processing video file -> " << input;
    #endif

    QFile inputFile(input);
    qreal inputSize = inputFile.size();

    if (inputSize > 1000000) {
        QString tempFile = QDir::tempPath() + "/" + code + ".mp4";
        if (QFile::exists(tempFile)) {
            if (!QFile::remove(tempFile)) {
                #ifdef TUP_DEBUG
                    qDebug() << "[ProjectManager::resizeVideo()] - Error while removing tmp file -> " << tempFile;
                    qDebug() << "[FAILED]";
                #endif
                return false;
            }
        }

        int width = 720;
        int height = (width * size.height()) / size.width();
        if (height % 2 != 0)
            height++;
        QString scale = "scale=" + QString::number(width) + ":" + QString::number(height);
#ifdef Q_OS_WIN
        QString program = "ffmpeg";
#else
        QString program = "/usr/bin/ffmpeg";
#endif
        QStringList arguments;
        arguments << "-i" << input << "-vf" << scale << tempFile;
        QString originalSize = QString::number(inputFile.size());

        #ifdef TUP_DEBUG
           qDebug() << "[ProjectManager::resizeVideo()] - Starting resizing process...";
        #endif
        QProcess *process = new QProcess(this);
        process->setProcessChannelMode(QProcess::MergedChannels);
        int exitStatus = process->execute(program, arguments);

        if (exitStatus != QProcess::NormalExit) {
            #ifdef TUP_DEBUG
                qDebug() << "  [ProjectManager::resizeVideo()] - Resizing process have failed absolutely!";
                qDebug() << "  [FAILED]";
            #endif
            return false;
        } else {
            if (QFile::remove(input)) {
                if (!QFile::copy(tempFile, input)) {
                    #ifdef TUP_DEBUG
                        qDebug() << "  [ProjectManager::resizeVideo()] - Error while copying output file!";
                        qDebug() << "  tempFile -> " << tempFile;
                        qDebug() << "  input -> " << input;
                        qDebug() << "  [FAILED]";
                    #endif
                    return false;
                } else {
                    QFile inputFile(input);
                    if (inputFile.size() > 0) {
                        if (!QFile::remove(tempFile)) {
                            #ifdef TUP_DEBUG
                                qDebug() << "  [ProjectManager::resizeVideo()] - Error while removing tmp file! -> " << tempFile;
                                qDebug() << "  [FAILED]";
                            #endif
                            return false;
                        } else {
                            #ifdef TUP_DEBUG
                                qDebug() << "  Original file size -> " << originalSize;
                                qDebug() << "  New file size -> " << QString::number(inputFile.size());
                                qDebug() << "  [SUCCESS]";
                            #endif
                        }
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "  [ProjectManager::resizeVideo()] - Ooops! Something went wrong! -> " << input;
                            qDebug() << "  [FAILED]";
                        #endif
                        return false;
                    }
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "  [ProjectManager::resizeVideo()] - Can't ovewrite video file! -> " << input;
                    qDebug() << "  [FAILED]";
                #endif
                return false;
            }
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[ProjectManager::resizeVideo()] - Video file is too small to be processed";
            qDebug() << "[ProjectManager::resizeVideo()] - Input file size -> " << QString::number(inputSize);
        #endif
    }

    return true;
}
