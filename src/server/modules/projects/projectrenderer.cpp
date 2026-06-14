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
#include "tupexportpluginobject.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPluginLoader>
#include <QProcess>

#include <cmath>

ProjectRenderer::ProjectRenderer(DatabaseHandler *dbHandler, QObject *parent)
    : QObject(parent), m_dbHandler(dbHandler), m_videoExporter(nullptr), m_imageExporter(nullptr)
{
    loadVideoPlugin();
    loadImagePlugin();
}

bool ProjectRenderer::isReady() const
{
    return isVideoReady() && isImageReady();
}

bool ProjectRenderer::isVideoReady() const
{
    return m_videoExporter != nullptr;
}

bool ProjectRenderer::isImageReady() const
{
    return m_imageExporter != nullptr;
}

void ProjectRenderer::loadVideoPlugin()
{
#ifdef Q_OS_WIN
    const QString expectedPlugin = "tupiffmpegplugin.dll";
#else
    const QString expectedPlugin = "libtupiffmpegplugin.so";
#endif

    loadExportPlugin(expectedPlugin, "video export", &m_videoExporter);
}

void ProjectRenderer::loadImagePlugin()
{
#ifdef Q_OS_WIN
    const QString expectedPlugin = "tupiimageplugin.dll";
#else
    const QString expectedPlugin = "libtupiimageplugin.so";
#endif

    loadExportPlugin(expectedPlugin, "image export", &m_imageExporter);
}

bool ProjectRenderer::loadExportPlugin(const QString &expectedPlugin,
                                       const QString &pluginLabel,
                                       TupExportInterface **targetExporter)
{
    if (!targetExporter) {
        qCritical() << "[ProjectRenderer::loadExportPlugin()] - Invalid exporter target for"
                    << pluginLabel;
        return false;
    }

    *targetExporter = nullptr;

#ifdef TUP_DEBUG
    qDebug() << "[ProjectRenderer::loadExportPlugin()] - Loading" << pluginLabel
             << "plugin from:" << PLUGINS_DIR;
#endif

    QDir pluginDirectory = QDir(PLUGINS_DIR);
    if (!pluginDirectory.exists()) {
        qCritical() << "[ProjectRenderer::loadExportPlugin()] - Plugin directory does not exist:"
                    << PLUGINS_DIR;
        return false;
    }

    bool found = false;
    foreach (QString fileName, pluginDirectory.entryList(QDir::Files)) {
        if (fileName.compare(expectedPlugin) != 0)
            continue;

        found = true;

#ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::loadExportPlugin()] -" << pluginLabel
                 << "plugin found. Loading:" << fileName;
#endif

        const QString pluginPath = pluginDirectory.absoluteFilePath(fileName);
        QPluginLoader loader(pluginPath);
        QObject *instance = loader.instance();
        if (!instance) {
            qCritical() << "[ProjectRenderer::loadExportPlugin()] - Could not load"
                        << pluginLabel << "plugin:" << pluginPath
                        << "error:" << loader.errorString();
            return false;
        }

        TupExportPluginObject *plugin = qobject_cast<TupExportPluginObject *>(instance);
        if (!plugin) {
            qCritical() << "[ProjectRenderer::loadExportPlugin()] -" << pluginLabel
                        << "plugin has invalid type:" << pluginPath;
            return false;
        }

        TupExportInterface *exporter = qobject_cast<TupExportInterface *>(plugin);
        if (!exporter) {
            qCritical() << "[ProjectRenderer::loadExportPlugin()] -" << pluginLabel
                        << "plugin does not implement TupExportInterface:" << pluginPath;
            return false;
        }

        *targetExporter = exporter;

#ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::loadExportPlugin()] -" << pluginLabel
                 << "plugin loaded:" << pluginPath;
#endif
        return true;
    }

    if (!found) {
        qCritical() << "[ProjectRenderer::loadExportPlugin()] - Fatal Error:" << pluginLabel
                    << "plugin was not found. Expected:" << expectedPlugin
                    << "directory:" << PLUGINS_DIR
                    << "available files:" << pluginDirectory.entryList(QDir::Files);
    }

    return false;
}

double ProjectRenderer::calculateDuration(TupProject *project, QList<TupScene *> &outSceneList)
{
    outSceneList.clear();

    int total = project->scenesCount();
    double timer = 0;

    for (int i = 0; i < total; i++) {
        TupScene *scene = project->sceneAt(i);
        if (!scene)
            continue;

        // Duration is calculated as frames / FPS for each scene.
        // Both values are required: frames gives the amount of animation data,
        // FPS tells how many frames are displayed per second.
        int fps = scene->getFPS();
        int frames = scene->framesCount();
        if (fps > 0) {
            timer += static_cast<double>(frames) / static_cast<double>(fps);
        } else if (frames > 0) {
            qWarning() << "[ProjectRenderer::calculateDuration()] - Invalid FPS in scene:"
                       << i << "fps:" << fps;
        }

        outSceneList.append(scene);
    }

    return timer;
}

QSize ProjectRenderer::normalizeVideoDimension(const QSize &size) const
{
    int w = size.width();
    int h = size.height();

    // MP4/H.264 export requires even dimensions in our pipeline.
    // Odd dimensions can cause chroma alignment issues and visible
    // red/blue color artifacts.
    if (w % 2)
        ++w;

    if (h % 2)
        ++h;

    return QSize(w, h);
}

bool ProjectRenderer::isSingleFrameProject(const QList<TupScene *> &sceneList) const
{
    return sceneList.count() == 1
           && sceneList.first()
           && sceneList.first()->framesCount() == 1;
}

bool ProjectRenderer::renderImage(TupProject *project,
                                  TupScene *scene,
                                  const QString &imagePath,
                                  int frameIndex,
                                  const QSize &dimension,
                                  QString &errorMessage)
{
    if (!project || !scene) {
        errorMessage = tr("Invalid project or scene for image export.");
        return false;
    }

    if (dimension.isEmpty()) {
        errorMessage = tr("Invalid image dimensions.");
        return false;
    }

    if (!m_imageExporter) {
        errorMessage = tr("Image export plugin is missing or could not be loaded. "
                          "The installation may be incomplete or corrupted.");
        qCritical() << "[ProjectRenderer::renderImage()] -" << errorMessage;
        return false;
    }

    bool ok = m_imageExporter->exportFrame(frameIndex,
                                           project->getCurrentBgColor(),
                                           imagePath,
                                           scene,
                                           dimension,
                                           project,
                                           true);
    if (!ok) {
        errorMessage = tr("Image export plugin failed.");
        qWarning() << "[ProjectRenderer::renderImage()] -" << errorMessage
                   << "Path:" << imagePath;
        return false;
    }

    return true;
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

    // --- 2. Locate the canonical .tup file ---
    // <ProjectsPath>/<studentId>/projects/<filename>/<filename>.tup
    TCONFIG->beginGroup("Projects");
    QString studentsDir = TCONFIG->value("ProjectsPath").toString();
    TCONFIG->endGroup();

    if (studentsDir.isEmpty())
        studentsDir = kAppProp->repositoryDir();

    QString projectDirPath = QDir(studentsDir).filePath(studentIdStr + "/projects/" + filename);
    QString tupPath = QDir(projectDirPath).filePath(filename + ".tup");

    if (!QFile::exists(tupPath)) {
        result.errorMessage = tr("Project file not found for '%1'. Expected path: %2")
                                  .arg(filename, tupPath);
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

    FileManager manager;
    bool loaded = manager.load(filename, project, studentIdStr);

    if (!loaded || project->scenesCount() == 0) {
        result.errorMessage = tr("Failed to load project '%1' or project has no scenes.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    // --- 4. Prepare scene list and duration ---
    QList<TupScene *> sceneList;
    double timer = calculateDuration(project, sceneList);

    if (sceneList.isEmpty()) {
        result.errorMessage = tr("Project '%1' has no renderable scenes.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    // --- 5. Prepare output directory ---
    TCONFIG->beginGroup("Render");
    QString renderPath = TCONFIG->value("RenderPath").toString();
    TCONFIG->endGroup();

    if (renderPath.isEmpty())
        renderPath = kAppProp->repositoryDir() + "/render";

    QString outDir = QDir(renderPath).filePath(studentIdStr);
    QDir dir;
    if (!dir.exists(outDir)) {
        if (!dir.mkpath(outDir)) {
            result.errorMessage = tr("Cannot create render output directory: %1").arg(outDir);
            qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
            delete project;
            return result;
        }
    }

    QSize dimension = normalizeVideoDimension(project->getDimension());

    // --- 6. Render illustration projects as PNG ---
    if (isSingleFrameProject(sceneList)) {
        QString imagePath = QDir(outDir).filePath(filename + ".png");
        QString imageError;

#ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::renderProject()] - Exporting illustration to PNG:"
                 << imagePath << "| Size:" << dimension;
#endif

        if (!renderImage(project, sceneList.first(), imagePath, 0, dimension, imageError)) {
            result.errorMessage = tr("Image export failed for project '%1'. %2")
                                      .arg(filename, imageError);
            qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
            delete project;
            return result;
        }

        m_dbHandler->updateProjectLastRendered(projectId);
        delete project;

        result.success = true;
        result.outputType = RenderResult::ImageOutput;
        result.outputPath = imagePath;
        result.imagePath = imagePath;

#ifdef TUP_DEBUG
        qDebug() << "[ProjectRenderer::renderProject()] - Image render complete:" << imagePath;
#endif
        return result;
    }

    // --- 7. Validate animation duration ---
    if (timer <= 0) {
        result.errorMessage = tr("Project '%1' has a duration of zero. "
                                 "Make sure your scenes have frames and a valid FPS value.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    // Very short animations are repeated so the generated video is easier
    // to perceive when played. This only affects the temporary render list;
    // the original project is not modified.
    if (timer > 0 && timer < 3.0) {
        int repetitions = static_cast<int>(std::ceil(3.0 / timer));
        QList<TupScene *> base = sceneList;
        for (int i = 1; i < repetitions; ++i)
            sceneList.append(base);
    }

    // --- 8. Delegate MP4 export to the ffmpeg plugin ---
    if (!m_videoExporter) {
        result.errorMessage = tr("Video export plugin is missing or could not be loaded. "
                                 "The installation may be incomplete or corrupted.");
        qCritical() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    int fps = sceneList.at(0)->getFPS();
    if (fps <= 0)
        fps = 24;

    QString mp4Path = QDir(outDir).filePath(filename + ".mp4");

#ifdef TUP_DEBUG
    qDebug() << "[ProjectRenderer::renderProject()] - Exporting to MP4:" << mp4Path;
    qDebug() << "[ProjectRenderer::renderProject()] - Scenes:" << sceneList.count()
             << "| FPS:" << fps << "| Size:" << dimension;
#endif

    bool isOk = m_videoExporter->exportToFormat(project->getCurrentBgColor().rgba(), mp4Path, sceneList,
                                                 TupExportInterface::MP4, dimension, dimension,
                                                 fps, project, true);
    if (!isOk) {
        result.errorMessage = tr("Video export failed for project '%1'.").arg(filename);
        qWarning() << "[ProjectRenderer::renderProject()] -" << result.errorMessage;
        delete project;
        return result;
    }

    // resizeVideo() is intentionally not called here.
    // The exporter already receives the normalized final dimension.
    // The legacy resize path is kept available for future requirements,
    // but running it here would unnecessarily reprocess a valid export.

    // Thumbnail generation was removed from this flow. If video thumbnails are
    // needed later, use renderImage() with a distinct thumbnail path.

    // --- 9. Update DB ---
    m_dbHandler->updateProjectLastRendered(projectId);

    delete project;

    result.success = true;
    result.outputType = RenderResult::VideoOutput;
    result.outputPath = mp4Path;
    result.mp4Path = mp4Path;

#ifdef TUP_DEBUG
    qDebug() << "[ProjectRenderer::renderProject()] - Render complete:" << mp4Path;
#endif

    return result;
}
