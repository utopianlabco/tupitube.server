/***************************************************************************
 *   Project TupiTube Server                                               *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Utopian Lab Development Team                                         *
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
#include "projectrenderer.h"

#include "netproject.h"
#include "filemanager.h"
#include "tapplicationproperties.h"
#include "tconfig.h"
#include "tupscene.h"
#include "genericexportplugin.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QPluginLoader>
#include <QDebug>

ProjectRenderer::ProjectRenderer(DatabaseHandler *dbHandler, QObject *parent)
    : QObject(parent), m_dbHandler(dbHandler), m_exporter(nullptr)
{
    loadVideoPlugin();
}

void ProjectRenderer::loadVideoPlugin()
{
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::loadVideoPlugin()] - Loading plugin from:" << PLUGINS_DIR;
    #endif

    QDir pluginDirectory(PLUGINS_DIR);
    foreach (const QString &fileName, pluginDirectory.entryList(QDir::Files)) {
        #ifdef Q_OS_WIN
            if (fileName.compare("tupiffmpegplugin.dll") != 0)
        #else
            if (fileName.compare("libtupiffmpegplugin.so") != 0)
        #endif
            continue;

        QPluginLoader loader(pluginDirectory.absoluteFilePath(fileName));
        TupExportPluginObject *plugin = qobject_cast<TupExportPluginObject *>(loader.instance());
        if (plugin) {
            m_exporter = qobject_cast<TupExportInterface *>(plugin);
            if (m_exporter) {
                #ifdef TUP_DEBUG
                    qDebug() << "[ProjectRenderer::loadVideoPlugin()] - Plugin loaded:" << fileName;
                #endif
                return;
            }
        }
        break;
    }

    qWarning() << "[ProjectRenderer::loadVideoPlugin()] - Fatal Error: ffmpeg plugin not found in:" << PLUGINS_DIR;
}

double ProjectRenderer::calculateDuration(TupProject *project,
                                          QList<TupScene *> &outSceneList,
                                          int &outThumbScene, int &outThumbFrame)
{
    outSceneList.clear();
    outThumbScene = 0;
    outThumbFrame = 0;

    int total = project->scenesCount();
    int middle = total / 2;
    double timer = 0;

    for (int i = 0; i < total; i++) {
        TupScene *scene = project->sceneAt(i);
        if (!scene)
            continue;

        int fps = scene->getFPS();
        int frames = scene->framesCount();
        if (fps > 0)
            timer += static_cast<double>(frames) / static_cast<double>(fps);

        outSceneList.append(scene);

        if (i == middle) {
            outThumbScene = i;
            outThumbFrame = frames / 2;
        }
    }

    return timer;
}

bool ProjectRenderer::resizeVideo(const QString &code, const QString &input, const QSize &size)
{
    if (size.isEmpty() || !QFile::exists(input))
        return false;

    int w = size.width();
    int h = size.height();

    qint64 inputSize = QFile(input).size();
    if (inputSize < 4 * 1024) {
        #ifdef TUP_DEBUG
            qDebug() << "[ProjectRenderer::resizeVideo()] - File too small to resize:" << input;
        #endif
        return true; // not an error; just skip resizing
    }

    QString tempFile = QDir::tempPath() + "/" + code + "_resized.mp4";
    QFile::remove(tempFile);

    #ifdef Q_OS_WIN
        QString program = "ffmpeg";
    #else
        QString program = "/usr/bin/ffmpeg";
    #endif

    QStringList args;
    args << "-y" << "-i" << input
         << "-vf" << QString("scale=%1:%2").arg(w).arg(h)
         << "-c:v" << "libx264" << "-preset" << "fast" << "-crf" << "23"
         << tempFile;

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, args);
    process.waitForFinished(-1);

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "[ProjectRenderer::resizeVideo()] - ffmpeg resize failed:" << process.readAll();
        QFile::remove(tempFile);
        return false;
    }

    QFile::remove(input);
    if (!QFile::rename(tempFile, input)) {
        qWarning() << "[ProjectRenderer::resizeVideo()] - Could not rename resized file to:" << input;
        return false;
    }

    return true;
}

ProjectRenderer::RenderResult ProjectRenderer::renderProject(int projectId)
{
    RenderResult result;
    result.success = false;

    // --- Guard: plugin must be loaded ---
    if (!m_exporter) {
        result.errorMessage = tr("Video export plugin is not loaded. "
                                 "Check the plugins directory.");
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        return result;
    }

    // --- 1. Fetch project info from DB ---
    DatabaseHandler::RenderProjectInfo info = m_dbHandler->getProjectRenderInfo(projectId);
    if (!info.found) {
        result.errorMessage = tr("Project %1 not found in database.").arg(projectId);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        return result;
    }

    QString studentIdStr = QString::number(info.studentId);
    QString filename = info.filename;

    #ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::renderProject()] - Rendering project:" << filename
                 << "owner:" << studentIdStr;
    #endif

    // --- 2. Locate the .tup file ---
    // Path variant A (student-saved): <students>/<uid>/projects/<filename>/<filename>.tup
    // Path variant B (teacher-created): <students>/<uid>/projects/<filename>.tup
    TCONFIG->beginGroup("Projects");
    QString studentsDir = TCONFIG->value("ProjectsPath").toString();
    TCONFIG->endGroup();

    if (studentsDir.isEmpty())
        studentsDir = kAppProp->repositoryDir();

    QString tupPathA = studentsDir + "/" + studentIdStr + "/projects/"
                       + filename + "/" + filename + ".tup";
    QString tupPathB = studentsDir + "/" + studentIdStr + "/projects/"
                       + filename + ".tup";

    QString tupPath;
    if (QFile::exists(tupPathA))
        tupPath = tupPathA;
    else if (QFile::exists(tupPathB))
        tupPath = tupPathB;
    else {
        result.errorMessage = tr("Project file not found for '%1' (tried '%2' and '%3').")
                                  .arg(filename).arg(tupPathA).arg(tupPathB);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        return result;
    }

    #ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::renderProject()] - Loading .tup file:" << tupPath;
    #endif

    // --- 3. Load the project ---
    NetProject *project = new NetProject(this);
    project->setFilename(filename);
    project->setOwner(info.studentId);

    FileManager *manager = new FileManager();
    bool loaded = manager->load(filename, project, studentIdStr);
    delete manager;

    if (!loaded || project->scenesCount() == 0) {
        result.errorMessage = tr("Failed to load project '%1' or project has no scenes.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    // --- 4. Prepare scene list and duration ---
    QList<TupScene *> sceneList;
    int thumbScene = 0;
    int thumbFrame = 0;
    double timer = calculateDuration(project, sceneList, thumbScene, thumbFrame);

    if (sceneList.isEmpty()) {
        result.errorMessage = tr("Project '%1' has no renderable scenes.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    if (timer <= 0) {
        result.errorMessage = tr("Project '%1' has a duration of zero. "
                                 "Make sure your scenes have frames and a valid FPS value.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    // Extend very short animations to at least 3 seconds
    if (timer > 0 && timer < 3.0) {
        int times = static_cast<int>(3.0 / timer) + 1;
        QList<TupScene *> base = sceneList;
        for (int i = 0; i < times; i++)
            sceneList.append(base);
    }

    // --- 5. Prepare output directory ---
    TCONFIG->beginGroup("Render");
    QString renderPath = TCONFIG->value("RenderPath").toString();
    TCONFIG->endGroup();

    if (renderPath.isEmpty())
        renderPath = kAppProp->repositoryDir() + "/render";

    QString outDir = renderPath + "/" + studentIdStr;
    QDir dir;
    if (!dir.exists(outDir)) {
        if (!dir.mkpath(outDir)) {
            result.errorMessage = tr("Cannot create render output directory: %1").arg(outDir);
            qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
            delete project;
            return result;
        }
    }

    QString mp4Path = outDir + "/" + filename + ".mp4";

    // --- 6. Prepare dimension (ensure even pixel counts for H.264) ---
    QSize dimension = project->getDimension();
    int w = dimension.width();
    int h = dimension.height();
    if (w % 2) w++;
    if (h % 2) h++;
    dimension = QSize(w, h);

    int fps = sceneList.at(0)->getFPS();
    if (fps <= 0)
        fps = 24;

    // --- 7. Export to MP4 ---
    #ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::renderProject()] - Exporting to MP4:" << mp4Path;
        qDebug() << "[ProjectRenderer::renderProject()] - Scenes:" << sceneList.count()
                 << "| FPS:" << fps << "| Size:" << dimension;
    #endif

    bool isOk = m_exporter->exportToFormat(project->getCurrentBgColor().rgba(), mp4Path, sceneList,
                                            TupExportInterface::MP4, dimension, dimension,
                                            fps, project, true);
    if (!isOk) {
        result.errorMessage = tr("Video export failed for project '%1'.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    // --- 8. Optional resize (normalize video dimensions) ---
    resizeVideo(filename, mp4Path, dimension);

    // --- 9. Create thumbnail ---
    QString thumbPath = outDir + "/" + filename + ".png";
    GenericExportPlugin imageExporter;
    bool thumbOk = imageExporter.exportFrame(thumbFrame,
                                              project->getCurrentBgColor(),
                                              thumbPath,
                                              project->sceneAt(thumbScene),
                                              dimension,
                                              project, true);
    if (!thumbOk) {
        // Non-fatal: thumbnail failure does not abort the render
        qWarning() << "[ProjectRenderer::renderProject()] - Warning: thumbnail creation failed for"
                 << filename;
    }

    // --- 10. Update DB ---
    m_dbHandler->updateProjectLastRendered(projectId);

    delete project;

    result.success = true;
    result.mp4Path = mp4Path;

    #ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::renderProject()] - Render complete:" << mp4Path;
    #endif

    return result;
}
