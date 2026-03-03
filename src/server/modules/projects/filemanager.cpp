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
#include "filemanager.h"
#include "netproject.h"
#include "tupscene.h"
#include "tuplibrary.h"
#include "packagehandler.h"
#include "talgorithm.h"
#include "tapplicationproperties.h"

#include <QDir>
#include <QDebug>

FileManager::FileManager() : QObject()
{
}

FileManager::~FileManager()
{
}

bool FileManager::save(const QString &filename, NetProject *project, int uid)
{
    #ifdef TUP_DEBUG
        qDebug() << "[FileManager::save()]";
    #endif

    // Ensure repositoryDir doesn't have trailing slash before building path
    QString repoDir = kAppProp->repositoryDir();
    if (repoDir.endsWith("/"))
        repoDir.chop(1);
    QString userPath = repoDir + "/users/" + QString::number(uid) + "/";

    QDir repository(userPath);
    if (!repository.exists()) {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::save()] - Creating user directories:" << userPath;
        #endif
        // Use mkpath to create the entire directory tree recursively
        bool ok = repository.mkpath(userPath + "projects");
        ok = ok && repository.mkpath(userPath + "animations/thumbnails");
        ok = ok && repository.mkpath(userPath + "storyboards/thumbnails");
        ok = ok && repository.mkpath(userPath + "images/thumbnails");

        #ifdef TUP_DEBUG
            if (ok)
                qWarning() << "[FileManager::save()] - User directories created successfully!";
            else
                qDebug() << "[FileManager::save()] - Failed to create user directories!";
        #endif
        
        if (!ok)
            return false;
    }

    QString absolutePath = userPath + "projects/" + filename + ".tup";

    // Ensure CACHE_DIR doesn't have trailing slash before adding uid
    QString cacheBase = CACHE_DIR;
    if (cacheBase.endsWith("/"))
        cacheBase.chop(1);
    QString cachePath = cacheBase + "/" + QString::number(uid);
    QDir cacheDir(cachePath);

    if (!cacheDir.exists()) {
        if (!cacheDir.mkpath(cacheDir.path())) {
            #ifdef TUP_DEBUG
                   qDebug() << "[FileManager::save()] - Fatal Error: Can't create path -> " << cacheDir.path();
            #endif
            return false;
        } 
    }

    cachePath = cacheDir.path() + "/" + filename;
    cacheDir = QDir(cachePath);

    if (!cacheDir.exists()) {
        #ifdef TUP_DEBUG
               qWarning() << "[FileManager::save()] - Creating project directory -> " << cacheDir.path();
        #endif

        if (!cacheDir.mkpath(cacheDir.path())) {
            #ifdef TUP_DEBUG
                   qDebug() << "[FileManager::save()] - Result: Epic Fail!";
            #endif
            return false;
        } else {
            #ifdef TUP_DEBUG
                   qWarning() << "[FileManager::save()] - Result: Successful!";
            #endif
        }
    }

    {
     // Save project
     QString tppPath = cacheDir.path() + QDir::separator() + "project.tpp";
     QFile projectFile(tppPath);

     if (projectFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
         QTextStream ts(&projectFile);
         QDomDocument doc;
         // project->setProjectName(name);
         doc.appendChild(project->toXml(doc));
         ts << doc.toString();
         projectFile.close();
     } else {
         #ifdef TUP_DEBUG
                qDebug() << "[FileManager::save()] - Fatal Error: Can't create file -> " << tppPath;
         #endif
     }
    }

    // Save scenes
    {
     int index = 0;
     // foreach (TupScene *scene, project->scenes().values()) {
     foreach (TupScene *scene, project->getScenes()) {
              QDomDocument doc;
              doc.appendChild(scene->toXml(doc));

              QString tpsPath = cacheDir.path() + QDir::separator() + "scene" + QString::number(index) + ".tps";
              QFile sceneFile(tpsPath);

              if (sceneFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                  QTextStream st(&sceneFile);
                  st << doc.toString();
                  index += 1;
                  sceneFile.close();
              } else {
                  #ifdef TUP_DEBUG
                         qDebug() << "[FileManager::save()] - Fatal Error: Can't create file -> " << tpsPath;
                  #endif
              }
     }
    }

    {
     // Save library
     QString tplPath = cacheDir.path() + QDir::separator() + "library.tpl";
     QFile libraryFile(tplPath);

     if (libraryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
         QTextStream ts(&libraryFile);

         QDomDocument doc;
         doc.appendChild(project->getLibrary()->toXml(doc));

         ts << doc.toString();
         libraryFile.close();
     } else {
         #ifdef TUP_DEBUG
                qDebug() << "[FileManager::save()] - Fatal Error: Can't create file -> " << tplPath;
         #endif
     }
    }

    PackageHandler packageHandler;
    bool isOk = packageHandler.makePackage(cacheDir.path(), absolutePath, QString::number(uid));

    #ifdef TUP_DEBUG
           qWarning() << "[FileManager::save()] - Saving project to -> " << absolutePath;
    #endif

    if (isOk) {
        #ifdef TUP_DEBUG
               qWarning() << "[FileManager::save()] - Result: Successful!";
        #endif
    } else {
        #ifdef TUP_DEBUG
               qDebug() << "[FileManager::save()] - Result: Epic Fail!";
        #endif
    }

    return isOk;
}

bool FileManager::load(const QString &filename, NetProject *project, const QString &uid)
{
    #ifdef TUP_DEBUG
        qDebug() << "[FileManager::load()]";
    #endif

    // Ensure repositoryDir doesn't have trailing slash before building path
    QString repoDir = kAppProp->repositoryDir();
    if (repoDir.endsWith("/"))
        repoDir.chop(1);
    QString absolutePath = repoDir + "/users/" + uid + "/projects/" + filename + ".tup";

    #ifdef TUP_DEBUG
        qWarning() << "[FileManager::load()] - Loading project -> " << absolutePath;
    #endif

    PackageHandler packageHandler;

    if (packageHandler.importPackage(absolutePath, uid)) {
        // Clear any default scenes created by NetProject constructor
        project->clear();

        QDir projectDir(packageHandler.importedProjectPath());
        QFile tppFile(projectDir.path() + QDir::separator() + "project.tpp");

        if (tppFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            project->fromXml(QString::fromLocal8Bit(tppFile.readAll()));
            tppFile.close();
        } else {
            #ifdef TUP_DEBUG
                   qDebug() << "[FileManager::load()] - Error while open .tpp file. Name: " << tppFile.fileName();
                   qDebug() << "[FileManager::load()] - Path: " << projectDir.path();
                   qDebug() << "[FileManager::load()] - Error Description: " << tppFile.errorString();
                   qDebug() << "[FileManager::load()] - Error type: " << tppFile.error();
            #endif
            return false;
        }

        project->setDataDir(packageHandler.importedProjectPath());
        project->loadLibrary(projectDir.path() + QDir::separator() + "library.tpl");

        QStringList scenes = projectDir.entryList(QStringList() << "*.tps", QDir::Readable | QDir::Files);

        if (scenes.count() > 0) {

            int index = 0;
            foreach (QString scenePath, scenes) {
                     scenePath = projectDir.path() + QDir::separator() + scenePath;

                     QFile file(scenePath);

                     if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                         QString xml = QString::fromLocal8Bit(file.readAll());
                         QDomDocument document;
                         if (! document.setContent(xml))
                             return false;
                         QDomElement root = document.documentElement();

                         #ifdef TUP_DEBUG
                                qWarning() << "[FileManager::load()] - Loading scene " << root.attribute("name");
                         #endif

                         TupScene *scene = project->createScene(root.attribute("name"), index, true);
                         scene->fromXml(xml);

                         index += 1;
                         file.close();
                     } else {
                         #ifdef TUP_DEBUG
                                qDebug() << "[FileManager::load()] - Fatal Error: Can't open file -> " << scenePath;
                         #endif
                         return false;
                     }
            }

            project->setOpen(true);

            return true;

        } else {
            #ifdef TUP_DEBUG
                   qDebug() << "[FileManager::load()] - Fatal Error: No scene files found (*.tps)";
            #endif
            return false;
        }
    }

    #ifdef TUP_DEBUG
           qDebug() << "[FileManager::load()] - Fatal Error: Can't import package -> " << filename;
    #endif

    return false;
}

bool FileManager::removeCacheDir(const QString &path)
{
    #ifdef TUP_DEBUG
        qDebug() << "FileManager::removeCacheDir() - Removing project path: " << path;
    #endif

    bool result = true;
    QDir dir(path);

    if (dir.exists(path)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden
                                                    | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                QString subDir = info.absoluteFilePath();
                if (CACHE_DIR.compare(subDir) != 0) {
                    result = removeCacheDir(subDir);
                } else {
                    #ifdef TUP_DEBUG
                       qWarning() << "[FileManager::removeCacheDir()] - Cache Path reached! -> " << subDir;
                    #endif
                    return true;
                }
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result)
                return result;
        }
        result = dir.rmdir(path);
    }

    #ifdef TUP_DEBUG
        qWarning() << "[FileManager::removeCacheDir()] - Result -> " + QString::number(result);
    #endif

    return result;
}

bool FileManager::createEmptyProjectFile(const QString &projectName, const QString &description,
                                          const QString &author, int ownerId, const QString &filename,
                                          const QSize &dimension, int fps)
{
    #ifdef TUP_DEBUG
        qDebug() << "[FileManager::createEmptyProjectFile()]";
        qWarning() << "[FileManager::createEmptyProjectFile()] - Creating project:" << projectName;
        qWarning() << "[FileManager::createEmptyProjectFile()] - Owner ID:" << ownerId;
        qWarning() << "[FileManager::createEmptyProjectFile()] - Filename:" << filename;
    #endif

    // Create the project structure
    NetProject *project = new NetProject();
    project->setProjectName(projectName);
    project->setDescription(description);
    project->setAuthor(author);
    project->setDimension(dimension);
    project->setFPS(fps);
    project->setFilename(filename);
    project->setOwner(ownerId);
    project->setOpen(true);

    // Create user directories
    QString repoDir = kAppProp->repositoryDir();
    if (repoDir.endsWith("/"))
        repoDir.chop(1);
    QString userPath = repoDir + "/users/" + QString::number(ownerId) + "/";

    QDir repository(userPath);
    if (!repository.exists()) {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Creating user directories:" << userPath;
        #endif
        bool ok = repository.mkpath(userPath + "projects");
        ok = ok && repository.mkpath(userPath + "animations/thumbnails");
        ok = ok && repository.mkpath(userPath + "storyboards/thumbnails");
        ok = ok && repository.mkpath(userPath + "images/thumbnails");

        if (!ok) {
            #ifdef TUP_DEBUG
                qWarning() << "[FileManager::createEmptyProjectFile()] - Failed to create user directories!";
            #endif
            delete project;
            return false;
        }
    }

    // Create cache directory for project
    QString cacheBase = CACHE_DIR;
    if (cacheBase.endsWith("/"))
        cacheBase.chop(1);
    QString cachePath = cacheBase + "/" + QString::number(ownerId) + "/" + filename;
    QDir cacheDir(cachePath);

    if (!cacheDir.exists()) {
        if (!cacheDir.mkpath(cachePath)) {
            #ifdef TUP_DEBUG
                qWarning() << "[FileManager::createEmptyProjectFile()] - Failed to create cache directory!";
            #endif
            delete project;
            return false;
        }
    }

    project->setDataDir(cachePath);

    // Save project.tpp
    QString tppPath = cachePath + "/project.tpp";
    QFile projectFile(tppPath);
    if (projectFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&projectFile);
        QDomDocument doc;
        doc.appendChild(project->toXml(doc));
        ts << doc.toString();
        projectFile.close();
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Can't create project.tpp";
        #endif
        delete project;
        return false;
    }

    // Save scene files
    int index = 0;
    foreach (TupScene *scene, project->getScenes()) {
        QDomDocument doc;
        doc.appendChild(scene->toXml(doc));

        QString tpsPath = cachePath + "/scene" + QString::number(index) + ".tps";
        QFile sceneFile(tpsPath);

        if (sceneFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream st(&sceneFile);
            st << doc.toString();
            index += 1;
            sceneFile.close();
        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[FileManager::createEmptyProjectFile()] - Can't create scene file";
            #endif
            delete project;
            return false;
        }
    }

    // Save library.tpl
    QString tplPath = cachePath + "/library.tpl";
    QFile libraryFile(tplPath);
    if (libraryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&libraryFile);
        QDomDocument doc;
        doc.appendChild(project->getLibrary()->toXml(doc));
        ts << doc.toString();
        libraryFile.close();
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Can't create library.tpl";
        #endif
        delete project;
        return false;
    }

    // Package into .tup file
    QString absolutePath = userPath + "projects/" + filename + ".tup";
    PackageHandler packageHandler;
    bool isOk = packageHandler.makePackage(cachePath, absolutePath, QString::number(ownerId));

    if (isOk) {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Project created successfully at:" << absolutePath;
        #endif
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Failed to create .tup package";
        #endif
    }

    delete project;
    return isOk;
}
