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
#include "genericexportplugin.h"
#include "tupexportinterface.h"
#include "tuprequestparser.h"
#include "filemanager.h"
#include "tupcommandexecutor.h"
#include "tupprojectcommand.h"
#include "server.h"
#include "global.h"
#include "project.h"
#include "user.h"
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
#include "tapplicationproperties.h"
#include "logger.h"

#include <QDir>
#include <QHash>
#include <QColor>
#include <QDebug>

// QString ProjectManager::BROWSER_FINGERPRINT = QString("TupiTube_Media 1.0");

ProjectManager::ProjectManager() : Observer()
{
    m_dbHandler = new DatabaseHandler();
    loadVideoPlugin();
}

ProjectManager::~ProjectManager()
{
}

void ProjectManager::createProject(Connection *connection)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::createProject()]";
    #endif
   
    int userID = connection->user()->uid(); 
    QString projectName = m_connectionData.name();

    QList<int> userList;
    userList.insert(0, userID);

    NetProject *project = new NetProject;
    project->setProjectParams(userID);
    project->setProjectName(projectName);
    project->setAuthor(m_connectionData.author());
    project->setDescription(m_connectionData.description());
    project->setCurrentBgColor(m_connectionData.bgColor());
    project->setDimension(m_connectionData.dimension());
    project->setFPS(m_connectionData.fps());
    project->setUsers(userList);

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
        QString uid = QString::number(userID);
        QString filename = project->filename();
        QString cacheDir = CACHE_DIR;
        if (cacheDir.endsWith("/"))
            cacheDir.chop(1);
        project->setDataDir(cacheDir + "/" + uid + "/" + filename);
        registerProject(connection, uid, filename, project);

        Logger::self()->info(QObject::tr("New project \"%1\" has been created by user %2").arg(projectName, connection->user()->login()));

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

void ProjectManager::openProject(const QString &filename, const QString &owner, Connection *connection)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::openProject()]";
        qWarning() << "[ProjectManager::openProject()] - Opening project: " << filename;
    #endif

    QString ownerID = m_dbHandler->userID(owner);
    QString projectID = m_dbHandler->exists(filename, ownerID);

    if (projectID.compare("-1") != 0) {
        if (!m_dbHandler->accessIsConfirmed(projectID, connection->user()->uid())) {
            #ifdef TUP_DEBUG
                   qDebug() << "[ProjectManager::openProject()] - Fatal Error: Insufficient permissions to access project -> " << filename;
                   qDebug() << "[ProjectManager::openProject()] - Request made by " << connection->user()->login();
            #endif
            connection->sendNotification(360, QObject::tr("Insufficient Permissions"),
                                         Notification::Error);
            connection->close();
            return;
        }

        NetProject *project = new NetProject;
        QObject::connect(project, SIGNAL(requestSendMessage(int, const QString&, Notification::Level)),
                         connection, SLOT(sendNotification(int, const QString&, Notification::Level)));
  
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

            Logger::self()->info(QObject::tr("Project \"%1\" has been openned by user %2").arg(project->getName(), connection->user()->login()));
        } else {
            #ifdef TUP_DEBUG
                   qWarning() << "[ProjectManager::openProject()] - Project is already open - Connecting socket...";
            #endif

            project = m_openedProjects.value(filename);
            project->save();

            QObject::connect(project, SIGNAL(requestSendMessage(int, const QString&, Notification::Level)),
                             connection, SLOT(sendNotification(int, const QString&, Notification::Level)));

            Logger::self()->info(QObject::tr("User %1 has joined the project \"%2\"").arg(connection->user()->login(), project->getName()));
        }

        registerProject(connection, ownerID, filename, project);
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
        int userID = connection->user()->uid();
        QString uid = QString::number(userID);

        NetProject *project = new NetProject;
        project->setProjectParams(userID);
        QString filename = project->filename();

        QString absolutePath = kAppProp->repositoryDir() + "users/" + uid + "/projects/" + filename + ".tup";
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
            Logger::self()->info(QObject::tr("Project \"%1\" has been imported by user %2").arg(project->getName(),
                                       connection->user()->login()));

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

void ProjectManager::registerProject(Connection *connection, const QString &uid, const QString &filename, NetProject *project)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::registerProject()]";
        qWarning() << "[ProjectManager::registerProject()] - Registering project with ID: " << filename;
    #endif

    project->setOpen(true);

    connection->setData(Info::ProjectID, filename);
    connection->setData(Info::ProjectIsOpen, true);
    m_openedProjects.insert(filename, project);

    QString absolutePath = kAppProp->repositoryDir() + "users/" + uid + "/projects/" + filename + ".tup";

    // Get project ID from database
    QString projectId = m_dbHandler->exists(filename, uid);
    
    // Build loginList from ALL project collaborators in database (not just connected users)
    QStringList userList;
    
    // Add project owner
    QString ownerUsername = m_dbHandler->getOwnerUsername(projectId.toInt());
    userList << ownerUsername;
    
    // Add all collaborators from database
    QList<DatabaseHandler::CollaboratorInfo> collaborators = m_dbHandler->getProjectCollaborators(projectId.toInt());
    for (const DatabaseHandler::CollaboratorInfo &collab : collaborators) {
        if (!userList.contains(collab.username))
            userList << collab.username;
    }
    
    QString loginList = userList.join(",");

    #ifdef TUP_DEBUG
        qWarning() << "[ProjectManager::registerProject()] - Login list: " << loginList;
    #endif

    Project projectPackage(loginList, absolutePath);
    connection->sendStringToClient(projectPackage.toString());

    // Send notice to connected partners
    QList<Connection *> partners = m_connectionList[filename];
    int size = partners.size();
    Notice msg(connection->user()->login(), 1);

    for (int i = 0; i < size; ++i)
         partners.at(i)->sendStringToClient(msg.toString());

    m_connectionList[filename].append(connection);
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
    QString uid = QString::number(connection->user()->uid());
    QString code = project->fileCode();
    QSize dimension = project->getDimension();

    QString path = kAppProp->repositoryDir() + "users/" + uid + "/images/" + code + ".png";
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

           QString thumb = kAppProp->repositoryDir() + "users/" + uid + "/images/thumbnails/" + code + ".png";
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

        QString login = connection->user()->login();
        Logger::self()->info(QObject::tr("Image %2.png has been exported by user \"%1\"").arg(login, code));
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
    QString uid = QString::number(connection->user()->uid());
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
                QString base = kAppProp->repositoryDir() + "users/" + uid + "/animations/" + code;

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

            QString fileName = kAppProp->repositoryDir() + "users/" + uid + "/animations/thumbnails/" + code + ".png";
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

            QString login = connection->user()->login();
            Logger::self()->info(QObject::tr("Video %2.webm has been exported by user \"%1\"").arg(login, code));
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
    QString uid = QString::number(connection->user()->uid());
    QString login = connection->user()->login();

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

        QString absolutePath = kAppProp->repositoryDir() + "users/" + uid + "/storyboards/" + directory;
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

                         QString thumb = kAppProp->repositoryDir() + "users/" + uid + "/storyboards/thumbnails/" + directory + ".png";
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

        Logger::self()->info(QObject::tr("Storyboard \"%1\" has been exported by user \"%2\"").arg(title, login));
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
        TupStoryboard *storyboard = new TupStoryboard(connection->user()->login());
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
        if (connection->user()->isEnabled()) {
            if (!connection->data(Info::ProjectID).toString().isNull()) {
                QString projectID = connection->data(Info::ProjectID).toString();
                if (handleProjectRequest(projectID, package)) {
                    QDomDocument request;
                    request.setContent(package);
                    sendToProjectMembers(connection, request);
                } else {
                    connection->sendNotification(340, QObject::tr("Cannot handle project request"), 
                                                 Notification::Warning);
                }
            } else {
                #ifdef TUP_DEBUG
                       qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Project ID undefined";
                #endif
            }

        } else {
            connection->sendNotification(360, QObject::tr("Insufficient permissions"), 
                                         Notification::Warning);
        }
    } else if (root == "project_open") {
               if (connection->user()->isEnabled()) {
                   OpenProjectParser parser;
                   if (parser.parse(package)) {
                       openProject(parser.projectID(), parser.owner(), connection);
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), 
                                                Notification::Warning);
               }
    } else if (root == "project_new") {
               if (connection->user()->isEnabled()) {
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
               if (connection->user()->isEnabled()) {
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
               if (connection->user()->isEnabled()) {
                   ListProjectsParser parser;
                   if (parser.parse(package)) {
                       listUserProjects(connection);
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_list package";
                       #endif
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), Notification::Warning);
               }
    } else if (root == "project_save") {
               if (connection->user()->isEnabled()) {
                   SaveProjectParser parser;
                   if (parser.parse(package)) {
                       QString projectID = connection->data(Info::ProjectID).toString();
                       if (m_openedProjects.contains(projectID)) {
                           if (!saveProject(projectID, parser.exit() == 1))
                               connection->sendNotification(381, QObject::tr("Error saving project %1").arg(projectID), 
                                                                 Notification::Error);
                       } else {
                           connection->sendNotification(321, QObject::tr("Project %1 doesn't exist!").arg(projectID), 
                                                        Notification::Error);
                       }
                   } else {
                       #ifdef TUP_DEBUG
                              qDebug() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_save package";
                       #endif
                   }
               } else {
                   connection->sendNotification(360, QObject::tr("Insufficient permissions"), Notification::Error);
               }
    } else if (root == "project_image") {
               if (connection->user()->isEnabled()) {
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
               if (connection->user()->isEnabled()) {
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
               if (connection->user()->isEnabled()) {
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
               if (connection->user()->isEnabled()) {
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
    m_connectionList.remove(projectID);

    Logger::self()->info(QObject::tr("Project \"%1\" has been closed").arg(projectID));
    #ifdef TUP_DEBUG
           qWarning() << "[ProjectManager::closeProject()] - Project " << projectID << " has been closed";
    #endif
}

bool ProjectManager::saveProject(const QString &projectID, bool quiet)
{
    if (m_openedProjects.contains(projectID))
        return m_openedProjects.value(projectID)->save(quiet);

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

        QString login = connection->user()->login();
        
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
            qWarning() << "[ProjectManager::closeConnection()] - User " << login << " has logged off";
        #endif
    }
}

bool ProjectManager::handleProjectRequest(const QString &projectID, const QString request)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::handleProjectRequest()]";
    #endif

    TupRequestParser parser;

    if (parser.parse(request)) {
        NetProject *project = m_openedProjects.value(projectID);
        if (project) {
            TupCommandExecutor *commandExecutor = new TupCommandExecutor(project);
            TupProjectCommand command(commandExecutor, parser.getResponse());
            command.redo();
            delete commandExecutor;
            project->resetTimer();

            return true;
        } else {
            #ifdef TUP_DEBUG
                   qDebug() << "[ProjectManager::handleProjectRequest()] - Fatal Error: Project " << projectID << " not found";
            #endif
        }
    }
    
    return false;
}

void ProjectManager::listUserProjects(Connection *connection)
{
    ProjectList list;
    int uid = connection->user()->uid();
    QString login = connection->user()->login();

    foreach (DatabaseHandler::ProjectInfo info, m_dbHandler->userProjects(uid, login))
             list.addProject(ProjectList::Work, info.file, info.title, info.owner, info.description, info.date);

    foreach (DatabaseHandler::ProjectInfo info, m_dbHandler->partnerProjects(uid))
             list.addProject(ProjectList::Contribution, info.file, info.title, info.owner, info.description, info.date);

    connection->sendStringToClient(list.toString());
}

void ProjectManager::sendToProjectMembers(Connection *connection, QDomDocument &doc)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectManager::sendToProjectMembers()]";
        qDebug() << "[ProjectManager::sendToProjectMembers()] - Sending request to clients...";
    #endif

    QString projectID = connection->data(Info::ProjectID).toString();
    connection->signPackage(doc);
 
    foreach (Connection *link, m_connectionList[projectID]) {
             if (link->user()->uid() != connection->user()->uid()) {
                 if (link->user()->isEnabled())
                     link->sendStringToClient(doc.toString(0));
             }
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
	if (fileName.compare("libtupiffmpegplugin.so") == 0) {
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
        // username (u)
        // filename (i)
        // type == "video" (p)
        // title (t)
        // topics (h)
        // description (d)
        // url: /?a=post&t=title&u=username&p=video&i=filename&h=topics&d=description

        QUrl serviceUrl = QUrl("http://dev.tupitube.co/api/index.php");
        QByteArray postData;
        postData.append("a=post&");
        postData.append("t=" + m_videoTitle + "&");
        postData.append("u=" + m_videoUsername + "&");
        postData.append("p=video&");
        postData.append("i=" + m_videoFilename + "&");
        postData.append("h=" + m_videoTopics + "&");
        postData.append("d=" + m_videoDescription);

        QString parameters(postData);
        qDebug() << "ProjectManager::postWork() - Post parameters: " << parameters;

        networkManager = new QNetworkAccessManager(this);
        connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(postFinished(QNetworkReply*)));

        QNetworkRequest request(serviceUrl);
        request.setRawHeader("User-Agent", BROWSER_FINGERPRINT.toLatin1());

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
