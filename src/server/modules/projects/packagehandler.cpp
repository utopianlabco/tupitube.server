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
#include "packagehandler.h"
#include "quazip.h"
#include "quazipfile.h"
#include "tapplicationproperties.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDebug>

PackageHandler::PackageHandler()
{
}

PackageHandler::~PackageHandler()
{
}

bool PackageHandler::makePackage(const QString &projectPath, const QString &packagePath, const QString &uid)
{
    m_uid = uid;
    m_importedProjectPath.clear();

    if (!QFileInfo(projectPath).exists()) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: Project path doesn't exist -> " << projectPath;
        #endif
        return false;
    }

    QuaZip zip(packagePath);

    if (!zip.open(QuaZip::mdCreate)) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: While creating package: " << zip.getZipError();
        #endif
        return false;
    }

    const bool compressed = compress(&zip, projectPath);
    zip.close();

    if (!compressed) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: While compressing project: " << zip.getZipError();
        #endif
        return false;
    }

    if (zip.getZipError() != UNZ_OK) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: Description: " << zip.getZipError();
        #endif
        return false;
    }

    return true;
}

bool PackageHandler::compress(QuaZip *zip, const QString &path)
{
    QDir dir(path);
    const QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files,
                                                    QDir::DirsFirst | QDir::Name);

    foreach (const QFileInfo &entry, entries) {
        if (entry.fileName().startsWith('.'))
            continue;

        if (entry.isDir()) {
            if (!compress(zip, entry.absoluteFilePath()))
                return false;
            continue;
        }

        if (!addFileToZip(zip, entry))
            return false;
    }

    return true;
}

bool PackageHandler::addFileToZip(QuaZip *zip, const QFileInfo &fileInfo)
{
    const QString entryName = archiveEntryName(fileInfo.absoluteFilePath());

    if (entryName.isEmpty()) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::addFileToZip()] - Fatal Error: Empty ZIP entry for file -> " << fileInfo.absoluteFilePath();
        #endif
        return false;
    }

    QFile inFile(fileInfo.absoluteFilePath());
    if (!inFile.open(QIODevice::ReadOnly)) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::addFileToZip()] - Fatal Error: While opening file ->  " << inFile.fileName()
                        << " - Description: " << inFile.errorString();
        #endif
        return false;
    }

    QuaZipFile outFile(zip);
    if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(entryName, fileInfo.absoluteFilePath())))
        return false;

    while (!inFile.atEnd()) {
        const QByteArray chunk = inFile.read(64 * 1024);
        if (chunk.isEmpty() && inFile.error() != QFile::NoError)
            return false;

        if (outFile.write(chunk) != chunk.size())
            return false;
    }

    outFile.close();
    inFile.close();

    return outFile.getZipError() == UNZ_OK;
}

QString PackageHandler::archiveEntryName(const QString &filePath) const
{
    // ZIP entry names are platform-independent and must always use '/'.
    QString entryName = stripRepositoryFromPath(filePath);
    entryName = QDir::fromNativeSeparators(entryName);

    while (entryName.startsWith('/'))
        entryName.remove(0, 1);

    return entryName;
}

QString PackageHandler::stripRepositoryFromPath(const QString &path) const
{
    QString normalizedPath = QDir::fromNativeSeparators(QFileInfo(path).absoluteFilePath());

    QString cacheBase = QDir::fromNativeSeparators(QFileInfo(CACHE_DIR).absoluteFilePath());
    while (cacheBase.endsWith('/'))
        cacheBase.chop(1);

    const QString prefix = cacheBase + "/" + m_uid + "/";

    if (normalizedPath.startsWith(prefix))
        normalizedPath.remove(0, prefix.length());

    return normalizedPath;
}

bool PackageHandler::importPackage(const QString &packagePath, const QString &uid)
{
    m_uid = uid;
    m_importedProjectPath.clear();

    QuaZip zip(packagePath);

    if (!zip.open(QuaZip::mdUnzip)) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::importPackage()] - Fatal Error: While opening package - Description: " << zip.getZipError();
        #endif
        return false;
    }

    zip.setFileNameCodec("IBM866"); // SQA: What is it?

    QuaZipFile file(&zip);
    QFile out;
    QuaZipFileInfo info;

    QString cacheDir = QDir::fromNativeSeparators(QFileInfo(CACHE_DIR).absoluteFilePath());
    while (cacheDir.endsWith('/'))
        cacheDir.chop(1);

    const QString importRoot = QDir(cacheDir).filePath(uid);
    QDir rootDir(importRoot);
    if (!rootDir.exists() && !rootDir.mkpath(rootDir.path())) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Can't create import root -> " << importRoot;
        #endif
        zip.close();
        return false;
    }

    bool next = zip.goToFirstFile();

    while (next) {
        if (!zip.getCurrentFileInfo(&info)) {
            #ifdef TUP_DEBUG
                   qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Can't get current file - Description: " << zip.getZipError();
            #endif
            zip.close();
            return false;
        }

        if (!file.open(QIODevice::ReadOnly)) {
            #ifdef TUP_DEBUG
                   qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Can't open file - Description: " << file.getZipError();
            #endif
            zip.close();
            return false;
        }

        QString entryName = QDir::fromNativeSeparators(file.getActualFileName());
        if (!isSafeArchiveEntryName(entryName)) {
            #ifdef TUP_DEBUG
                   qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Unsafe ZIP entry -> " << entryName;
            #endif
            file.close();
            zip.close();
            return false;
        }

        QString name = rootDir.filePath(entryName);
        name = QDir::cleanPath(name);

        if (name.endsWith('/'))
            name.chop(1);

        if (name.endsWith(".tpp"))
            m_importedProjectPath = QFileInfo(name).path();

        if (file.getZipError() != UNZ_OK) {
            #ifdef TUP_DEBUG
                   qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Error while open package - Description: " << file.getZipError();
            #endif
            file.close();
            zip.close();
            return false;
        }

        if (createPath(name)) {
            out.setFileName(name);

            if (!out.open(QIODevice::WriteOnly)) {
                #ifdef TUP_DEBUG
                       qDebug() << "[PackageHandler::importPackage()] - Error while open file -> " << out.fileName();
                       qDebug() << "[PackageHandler::importPackage()] - Error Description: " << out.errorString();
                       qDebug() << "[PackageHandler::importPackage()] - Error type: " << out.error();
                #endif
                file.close();
                zip.close();
                return false;
            }

            while (!file.atEnd()) {
                const QByteArray chunk = file.read(64 * 1024);
                if (chunk.isEmpty() && file.getZipError() != UNZ_OK)
                    break;
                out.write(chunk);
            }

            out.close();
        } else {
            #ifdef TUP_DEBUG
                   qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Error creating path -> " << name;
            #endif
            file.close();
            zip.close();
            return false;
        }

        if (file.getZipError() != UNZ_OK) {
            #ifdef TUP_DEBUG
                   qDebug() << "[PackageHandler::importPackage()] - Fatal Error: While opening package - Description: " << file.getZipError();
            #endif
            file.close();
            zip.close();
            return false;
        }

        file.close();

        if (file.getZipError() != UNZ_OK) {
            #ifdef TUP_DEBUG
                   qDebug() << "[PackageHandler::importPackage()] - Fatal Error: While opening package - Description: " << file.getZipError();
            #endif
            zip.close();
            return false;
        }

        next = zip.goToNextFile();
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::importPackage()] - Fatal Error: While opening package - Description: " << file.getZipError();
        #endif
        return false;
    }

    return true;
}

bool PackageHandler::createPath(const QString &filePath)
{
    QFileInfo info(filePath);
    QDir path = info.dir();
    const QString target = path.path();

    if (!path.exists())
        return path.mkpath(target);

    return true;
}

bool PackageHandler::isSafeArchiveEntryName(const QString &entryName) const
{
    if (entryName.isEmpty())
        return false;

    if (entryName.startsWith('/') || entryName.startsWith('\\'))
        return false;

    if (entryName.contains(":"))
        return false;

    const QString cleaned = QDir::cleanPath(entryName);
    if (cleaned == ".." || cleaned.startsWith("../") || cleaned.contains("/../"))
        return false;

    return true;
}

QString PackageHandler::importedProjectPath() const
{
    return m_importedProjectPath;
}
