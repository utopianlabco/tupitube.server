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
#include "tglobal.h"
#include "tapplicationproperties.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
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

    if (!QFile::exists(projectPath)) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: Project path doesn't exist -> " << projectPath;
        #endif
        return false;
    }
    
    QFileInfo packageInfo(packagePath);
    QuaZip zip(packagePath);

    if (!zip.open(QuaZip::mdCreate)) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: While creating package: " << zip.getZipError();
        #endif
        return false;
    }

    if (! compress(&zip, projectPath)) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: While compressing project: " << zip.getZipError();
        #endif
        return false;
    }
    
    zip.close();

    if (zip.getZipError() != 0) {
        #ifdef TUP_DEBUG
               qDebug() << "[PackageHandler::makePackage()] - Fatal Error: Description: " << zip.getZipError();
        #endif
        return false;
    }
    
    return true;
}

bool PackageHandler::compress(QuaZip *zip, const QString &path)
{
    QFile inFile;
    QuaZipFile outFile(zip);
    char c;

    QFileInfoList files = QDir(path).entryInfoList();
    
    foreach (QFileInfo file, files) {
             QString filePath = path + QDir::separator() + file.fileName();

             if (file.fileName().startsWith("."))
                 continue;
        
             if (file.isDir()) {
                 compress(zip, file.path() + QDir::separator() + file.fileName());
                 continue;
             }
        
             if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(stripRepositoryFromPath(filePath), stripRepositoryFromPath(filePath)))) 
                 return false;

             inFile.setFileName(filePath);

             if (!inFile.open(QIODevice::ReadOnly)) {
                 #ifdef TUP_DEBUG
                        qDebug() << "[PackageHandler::compress()] - Fatal Error: While opening file ->  " << inFile.fileName() << " - Description: " << inFile.errorString();
                 #endif
                 return false;
             }

             while (inFile.getChar(&c) && outFile.putChar(c)) {};
        
             if (outFile.getZipError()!=UNZ_OK)
                 return false;

             outFile.close();

             if (outFile.getZipError()!=UNZ_OK)
                 return false;

             inFile.close();
    }
    
    return true;
}

QString PackageHandler::stripRepositoryFromPath(QString path)
{
    // Remove the CACHE_DIR prefix and the uid folder, keeping only project_id/filename
    QString cacheBase = CACHE_DIR;
    if (cacheBase.endsWith("/"))
        cacheBase.chop(1);
    QString prefix = cacheBase + "/" + m_uid + "/";
    path.remove(prefix);

    return path;
}

bool PackageHandler::importPackage(const QString &packagePath, const QString &uid)
{
    m_uid = uid;

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
    QString name;
    char c;
    QuaZipFileInfo info;

    QString cacheDir = CACHE_DIR;
    if (cacheDir.endsWith("/"))
        cacheDir.chop(1);

    bool next = zip.goToFirstFile();

    while (next) {

           if (!zip.getCurrentFileInfo(&info)) {
               #ifdef TUP_DEBUG
                      qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Can't get current file - Description: " << zip.getZipError();
               #endif
               return false;
           }
        
           if (!file.open(QIODevice::ReadOnly)) {
               #ifdef TUP_DEBUG
                      qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Can't open file - Description: " << file.getZipError();
               #endif
               return false;
           }
        
           name = cacheDir + "/" + uid + "/" + file.getActualFileName();

           if (name.endsWith(QDir::separator()))
               name.remove(name.count()-1, 1);

           if (name.endsWith(".tpp"))
               m_importedProjectPath = QFileInfo(name).path();
        
           if (file.getZipError() != UNZ_OK) {
               #ifdef TUP_DEBUG
                      qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Error while open package - Description: " << file.getZipError();
               #endif
               return false;
           }
        
           if (createPath(name)) {

               out.setFileName(name);
        
               if (! out.open(QIODevice::WriteOnly)) {
                   #ifdef TUP_DEBUG
                          qDebug() << "[PackageHandler::importPackage()] - Error while open file -> " << out.fileName(); 
                          qDebug() << "[PackageHandler::importPackage()] - Error Description: " << out.errorString();
                          qDebug() << "[PackageHandler::importPackage()] - Error type: " << out.error(); 
                   #endif
                   return false;
               }
        
               while (file.getChar(&c)) 
                      out.putChar(c);

               out.close();
           } else {
               #ifdef TUP_DEBUG
                      qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Error creating path -> " << name; 
               #endif
               return false;
           }

           if (file.getZipError()!=UNZ_OK) {
               #ifdef TUP_DEBUG
                      qDebug() << "[PackageHandler::importPackage()] - Fatal Error: While opening package - Description: " << file.getZipError();
               #endif
               return false;
           }

           if (!file.atEnd()) {
               #ifdef TUP_DEBUG
                      qDebug() << "[PackageHandler::importPackage()] - Fatal Error: Not EOF Error";
               #endif
               return false;
           }

           file.close();

           if (file.getZipError()!=UNZ_OK) {
               #ifdef TUP_DEBUG
                      qDebug() << "[PackageHandler::importPackage()] - Fatal Error: While opening package - Description: " << file.getZipError();
               #endif
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
    QString target = path.path();
    
    if (!path.exists()) 
        return path.mkpath(target);
    else 
        return true;
    
    return false;
}

QString PackageHandler::importedProjectPath() const
{
    return m_importedProjectPath;
}
