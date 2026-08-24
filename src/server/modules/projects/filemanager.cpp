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

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>
#include <QDebug>
#include <QDomDocument>
#include <QTextStream>

namespace {

QString cleanBasePath(const QString &path)
{
    QString cleanPath = QDir::cleanPath(path);
    while (cleanPath.endsWith('/') || cleanPath.endsWith('\\'))
        cleanPath.chop(1);
    return cleanPath;
}

QString studentPathFor(int uid)
{
    const QString repoDir = cleanBasePath(kAppProp->repositoryDir());
    QString projectsDir = QDir(repoDir).filePath("projects");
    return QDir(projectsDir).filePath(QString::number(uid));
}

QString studentPathFor(const QString &uid)
{
    const QString repoDir = cleanBasePath(kAppProp->repositoryDir());
    QString projectsDir = QDir(repoDir).filePath("projects");
    return QDir(projectsDir).filePath(uid);
}

QString projectDirectoryPath(const QString &studentPath, const QString &filename)
{
    return QDir(QDir(studentPath).filePath("sources")).filePath(filename);
}

QString projectPackagePath(const QString &studentPath, const QString &filename)
{
    return QDir(projectDirectoryPath(studentPath, filename)).filePath(filename + ".tup");
}


QString pendingProjectPackagePath(const QString &studentPath, const QString &filename, qint64 revision)
{
    return QDir(projectDirectoryPath(studentPath, filename)).filePath(
        filename + ".pending-r" + QString::number(revision) + ".tup");
}

QString fileSha256(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        return QString();

    return QString::fromLatin1(hash.result().toHex());
}

bool copyFileAtomically(const QString &sourcePath, const QString &destinationPath)
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly))
        return false;

    QSaveFile destination(destinationPath);
    destination.setDirectWriteFallback(false);
    if (!destination.open(QIODevice::WriteOnly))
        return false;

    const qint64 bufferSize = 1024 * 1024;
    while (!source.atEnd()) {
        const QByteArray chunk = source.read(bufferSize);
        if (chunk.isEmpty() && source.error() != QFile::NoError) {
            destination.cancelWriting();
            return false;
        }

        if (destination.write(chunk) != chunk.size()) {
            destination.cancelWriting();
            return false;
        }
    }

    return destination.commit();
}

bool removePendingSnapshotsExcept(const QString &studentPath, const QString &filename,
                                  const QString &keepPath = QString())
{
    QDir projectDir(projectDirectoryPath(studentPath, filename));
    const QString pattern = filename + ".pending-r*.tup";
    const QStringList pendingFiles = projectDir.entryList(QStringList() << pattern, QDir::Files, QDir::Name);

    for (const QString &pendingFile : pendingFiles) {
        const QString pendingPath = projectDir.filePath(pendingFile);
        if (!keepPath.isEmpty() && QFileInfo(pendingPath) == QFileInfo(keepPath))
            continue;
        if (!QFile::remove(pendingPath))
            return false;
    }

    return true;
}

QString cacheRootPathFor(const QString &uid)
{
    const QString cacheBase = cleanBasePath(CACHE_DIR);
    return QDir(cacheBase).filePath(uid);
}

QString cacheProjectPathFor(const QString &uid, const QString &filename)
{
    return QDir(cacheRootPathFor(uid)).filePath(filename);
}

bool createStudentDirectories(const QString &studentPath)
{
    QDir dir;

    if (!dir.mkpath(QDir(studentPath).filePath("sources")))
        return false;

    if (!dir.mkpath(QDir(QDir(studentPath).filePath("animations")).filePath("thumbnails")))
        return false;

    if (!dir.mkpath(QDir(QDir(studentPath).filePath("storyboards")).filePath("thumbnails")))
        return false;

    if (!dir.mkpath(QDir(QDir(studentPath).filePath("images")).filePath("thumbnails")))
        return false;

    return true;
}

bool writeXmlFile(const QString &filePath, const QDomDocument &doc)
{
    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        #ifdef TUP_DEBUG
               qDebug() << "[FileManager] - Fatal Error: Can't create file -> " << filePath
                        << " - Description: " << file.errorString();
        #endif
        return false;
    }

    QTextStream ts(&file);
    ts << doc.toString();
    ts.flush();

    if (ts.status() != QTextStream::Ok) {
        #ifdef TUP_DEBUG
               qDebug() << "[FileManager] - Fatal Error: Can't write file -> " << filePath;
        #endif
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        #ifdef TUP_DEBUG
               qDebug() << "[FileManager] - Fatal Error: Can't commit file -> " << filePath
                        << " - Description: " << file.errorString();
        #endif
        return false;
    }

    return true;
}

bool saveProjectFiles(const QString &cachePath, NetProject *project)
{
    QDir cacheDir(cachePath);

    QDomDocument projectDoc;
    projectDoc.appendChild(project->toXml(projectDoc));
    if (!writeXmlFile(cacheDir.filePath("project.tpp"), projectDoc))
        return false;

    int index = 0;
    QSet<QString> expectedSceneFiles;
    foreach (TupScene *scene, project->getScenes()) {
        QDomDocument sceneDoc;
        sceneDoc.appendChild(scene->toXml(sceneDoc));

        const QString sceneFileName = "scene" + QString::number(index) + ".tps";
        const QString scenePath = cacheDir.filePath(sceneFileName);
        expectedSceneFiles.insert(sceneFileName);
        if (!writeXmlFile(scenePath, sceneDoc))
            return false;

        index += 1;
    }

    const QStringList existingSceneFiles = cacheDir.entryList(QStringList() << "scene*.tps",
                                                               QDir::Files, QDir::Name);
    for (const QString &sceneFile : existingSceneFiles) {
        if (!expectedSceneFiles.contains(sceneFile) && !QFile::remove(cacheDir.filePath(sceneFile)))
            return false;
    }

    QDomDocument libraryDoc;
    libraryDoc.appendChild(project->getLibrary()->toXml(libraryDoc));
    if (!writeXmlFile(cacheDir.filePath("library.tpl"), libraryDoc))
        return false;

    return true;
}

} // namespace

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

    if (!project) {
        #ifdef TUP_DEBUG
            qDebug() << "[FileManager::save()] - Fatal Error: Null project pointer";
        #endif
        return false;
    }

    const QString uidText = QString::number(uid);
    const QString studentPath = studentPathFor(uid);

    if (!createStudentDirectories(studentPath)) {
        #ifdef TUP_DEBUG
            qDebug() << "[FileManager::save()] - Failed to create student directories!";
        #endif
        return false;
    }

    const QString projectPath = projectDirectoryPath(studentPath, filename);
    QDir projectDir(projectPath);
    if (!projectDir.exists() && !projectDir.mkpath(projectDir.path())) {
        #ifdef TUP_DEBUG
            qDebug() << "[FileManager::save()] - Failed to create project directory -> " << projectPath;
        #endif
        return false;
    }

    const QString absolutePath = projectPackagePath(studentPath, filename);
    const QString cachePath = cacheProjectPathFor(uidText, filename);
    QDir cacheDir(cachePath);

    if (!cacheDir.exists()) {
        #ifdef TUP_DEBUG
               qWarning() << "[FileManager::save()] - Creating project cache directory -> " << cacheDir.path();
        #endif

        if (!cacheDir.mkpath(cacheDir.path())) {
            #ifdef TUP_DEBUG
                   qDebug() << "[FileManager::save()] - Result: Epic Fail!";
            #endif
            return false;
        }
    }

    if (!saveProjectFiles(cacheDir.path(), project))
        return false;

    PackageHandler packageHandler;
    const bool isOk = packageHandler.makePackage(cacheDir.path(), absolutePath, uidText);

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

bool FileManager::savePendingSnapshot(const QString &filename, NetProject *project, int uid,
                                      qint64 revision, QString *checksum)
{
    if (!project || revision <= 0)
        return false;

    const QString uidText = QString::number(uid);
    const QString studentPath = studentPathFor(uid);
    if (!createStudentDirectories(studentPath))
        return false;

    const QString projectPath = projectDirectoryPath(studentPath, filename);
    QDir projectDir(projectPath);
    if (!projectDir.exists() && !projectDir.mkpath(projectDir.path()))
        return false;

    const QString cachePath = cacheProjectPathFor(uidText, filename);
    QDir cacheDir(cachePath);
    if (!cacheDir.exists() && !cacheDir.mkpath(cacheDir.path()))
        return false;

    if (!saveProjectFiles(cacheDir.path(), project))
        return false;

    const QString pendingPath = pendingProjectPackagePath(studentPath, filename, revision);
    PackageHandler packageHandler;
    if (!packageHandler.makePackage(cacheDir.path(), pendingPath, uidText))
        return false;

    const QString digest = fileSha256(pendingPath);
    if (digest.isEmpty()) {
        QFile::remove(pendingPath);
        return false;
    }

    if (checksum)
        *checksum = digest;

#ifdef TUP_DEBUG
    qWarning() << "[FileManager::savePendingSnapshot()] - Durable pending snapshot ->"
               << pendingPath << "Revision:" << revision << "SHA-256:" << digest;
#endif
    return true;
}

bool FileManager::promotePendingSnapshot(const QString &filename, int uid, qint64 revision,
                                         const QString &expectedChecksum)
{
    if (revision <= 0)
        return false;

    const QString studentPath = studentPathFor(uid);
    const QString pendingPath = pendingProjectPackagePath(studentPath, filename, revision);
    const QString activePath = projectPackagePath(studentPath, filename);

    if (!QFileInfo::exists(pendingPath))
        return false;

    if (!expectedChecksum.isEmpty() && fileSha256(pendingPath) != expectedChecksum) {
#ifdef TUP_DEBUG
        qWarning() << "[FileManager::promotePendingSnapshot()] - Pending snapshot checksum mismatch ->"
                   << pendingPath;
#endif
        return false;
    }

    if (!copyFileAtomically(pendingPath, activePath))
        return false;

    if (!expectedChecksum.isEmpty() && fileSha256(activePath) != expectedChecksum)
        return false;

    if (!removePendingSnapshotsExcept(studentPath, filename)) {
#ifdef TUP_DEBUG
        qWarning() << "[FileManager::promotePendingSnapshot()] - Unable to remove one or more stale pending snapshots.";
#endif
    }

    return true;
}

bool FileManager::discardPendingSnapshot(const QString &filename, int uid, qint64 revision)
{
    if (revision <= 0)
        return false;

    const QString pendingPath = pendingProjectPackagePath(studentPathFor(uid), filename, revision);
    return !QFileInfo::exists(pendingPath) || QFile::remove(pendingPath);
}

bool FileManager::reconcileAuthoritativeSnapshot(const QString &filename, const QString &uid,
                                                 qint64 revision, const QString &expectedChecksum)
{
    if (revision < 0)
        return false;

    const QString studentPath = studentPathFor(uid);
    const QString activePath = projectPackagePath(studentPath, filename);
    const QString pendingPath = pendingProjectPackagePath(studentPath, filename, revision);

    if (revision > 0 && QFileInfo::exists(pendingPath)) {
        if (expectedChecksum.isEmpty() || fileSha256(pendingPath) != expectedChecksum) {
            qCritical() << "[FileManager::reconcileAuthoritativeSnapshot()]"
                        << "Committed pending snapshot checksum mismatch. Project:" << filename
                        << "Revision:" << revision;
            return false;
        }

        if (!copyFileAtomically(pendingPath, activePath)) {
            qCritical() << "[FileManager::reconcileAuthoritativeSnapshot()]"
                        << "Unable to promote committed pending snapshot. Project:" << filename
                        << "Revision:" << revision;
            return false;
        }

        if (fileSha256(activePath) != expectedChecksum) {
            qCritical() << "[FileManager::reconcileAuthoritativeSnapshot()]"
                        << "Promoted snapshot checksum mismatch. Project:" << filename
                        << "Revision:" << revision;
            return false;
        }

#ifdef TUP_DEBUG
        qWarning() << "[FileManager::reconcileAuthoritativeSnapshot()]"
                   << "Recovered committed snapshot from pending package. Project:" << filename
                   << "Revision:" << revision;
#endif
    }

    if (!QFileInfo::exists(activePath))
        return false;

    // A pending package for a revision newer than SQLite's committed revision
    // belongs to a command that never committed. Once the committed snapshot
    // is known to be available, those candidates are safe to discard. Cleanup
    // is best effort; stale candidates never become authoritative without a
    // matching committed revision/checksum in SQLite.
    if (!removePendingSnapshotsExcept(studentPath, filename)) {
#ifdef TUP_DEBUG
        qWarning() << "[FileManager::reconcileAuthoritativeSnapshot()] - Unable to remove one or more stale pending snapshots.";
#endif
    }

    return true;
}

bool FileManager::load(const QString &filename, NetProject *project, const QString &uid)
{
    #ifdef TUP_DEBUG
        qDebug() << "[FileManager::load()]";
    #endif

    if (!project) {
        #ifdef TUP_DEBUG
            qDebug() << "[FileManager::load()] - Fatal Error: Null project pointer";
        #endif
        return false;
    }

    const QString studentPath = studentPathFor(uid);
    const QString absolutePath = projectPackagePath(studentPath, filename);

    #ifdef TUP_DEBUG
        qWarning() << "[FileManager::load()] - Loading project -> " << absolutePath;
    #endif

    PackageHandler packageHandler;

    if (packageHandler.importPackage(absolutePath, uid)) {
        project->clear();

        QDir projectDir(packageHandler.importedProjectPath());
        QFile tppFile(projectDir.filePath("project.tpp"));

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
        project->loadLibrary(projectDir.filePath("library.tpl"));

        const QStringList scenes = projectDir.entryList(QStringList() << "*.tps",
                                                        QDir::Readable | QDir::Files,
                                                        QDir::Name);

        if (scenes.count() > 0) {
            int index = 0;

            foreach (QString sceneFileName, scenes) {
                const QString scenePath = projectDir.filePath(sceneFileName);
                QFile file(scenePath);

                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    const QString xml = QString::fromLocal8Bit(file.readAll());
                    QDomDocument document;

                    if (!document.setContent(xml))
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
        }

        #ifdef TUP_DEBUG
               qDebug() << "[FileManager::load()] - Fatal Error: No scene files found (*.tps)";
        #endif
        return false;
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
        const QString cacheRoot = QFileInfo(CACHE_DIR).canonicalFilePath();

        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden
                                                    | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                const QString subDir = info.absoluteFilePath();
                const QString canonicalSubDir = QFileInfo(subDir).canonicalFilePath();

                if (cacheRoot.isEmpty() || canonicalSubDir != cacheRoot) {
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

    NetProject *project = new NetProject();
    project->setProjectName(projectName);
    project->setDescription(description);
    project->setAuthor(author);
    project->setDimension(dimension);
    project->setFPS(fps);
    project->setFilename(filename);
    project->setOwner(ownerId);
    project->setOpen(true);

    const QString uidText = QString::number(ownerId);
    const QString studentPath = studentPathFor(ownerId);

    if (!createStudentDirectories(studentPath)) {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Failed to create student directories!";
        #endif
        delete project;
        return false;
    }

    const QString projectPath = projectDirectoryPath(studentPath, filename);
    QDir projectDir(projectPath);
    if (!projectDir.exists() && !projectDir.mkpath(projectDir.path())) {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Failed to create project directory!";
        #endif
        delete project;
        return false;
    }

    const QString cachePath = cacheProjectPathFor(uidText, filename);
    QDir cacheDir(cachePath);

    if (!cacheDir.exists() && !cacheDir.mkpath(cacheDir.path())) {
        #ifdef TUP_DEBUG
            qWarning() << "[FileManager::createEmptyProjectFile()] - Failed to create cache directory!";
        #endif
        delete project;
        return false;
    }

    project->setDataDir(cacheDir.path());

    if (!saveProjectFiles(cacheDir.path(), project)) {
        delete project;
        return false;
    }

    const QString absolutePath = projectPackagePath(studentPath, filename);
    PackageHandler packageHandler;
    const bool isOk = packageHandler.makePackage(cacheDir.path(), absolutePath, uidText);

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
