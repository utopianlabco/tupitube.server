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
#ifndef PROJECTRENDERER_H
#define PROJECTRENDERER_H

#include <QObject>
#include <QList>
#include <QSize>
#include <QString>
#include <QStringList>

#include "tupexportinterface.h"
#include "databasehandler.h"

class TupProject;
class TupScene;

class ProjectRenderer : public QObject
{
    Q_OBJECT

public:
    struct RenderResult {
        enum OutputType {
            NoOutput,
            VideoOutput,
            ImageOutput
        };

        bool success = false;
        OutputType outputType = NoOutput;
        QString outputPath;
        QString mp4Path;
        QString imagePath;
        QString errorMessage;
        QStringList warnings;
    };

    explicit ProjectRenderer(DatabaseHandler *dbHandler, QObject *parent = nullptr);

    bool isReady() const;
    bool isVideoReady() const;
    bool isImageReady() const;

    // Renders the given project_id to the configured render output directory.
    // Animation projects are exported as MP4 files. Illustration projects
    // consisting of exactly one scene with one frame are exported as PNG files.
    // Updates tupitube_project.last_rendered_at on success.
    // Thread-safe to call but blocks until rendering is complete.
    RenderResult renderProject(int projectId);

private:
    void loadVideoPlugin();
    void loadImagePlugin();
    bool loadExportPlugin(const QString &expectedPlugin,
                          const QString &pluginLabel,
                          TupExportInterface **targetExporter);

    double calculateDuration(TupProject *project, QList<TupScene *> &outSceneList);
    QSize normalizeVideoDimension(const QSize &size) const;
    bool isSingleFrameProject(const QList<TupScene *> &sceneList) const;
    bool renderImage(TupProject *project,
                     TupScene *scene,
                     const QString &imagePath,
                     int frameIndex,
                     const QSize &dimension,
                     QString &errorMessage);
    bool resizeVideo(const QString &code, const QString &input, const QSize &size);

    DatabaseHandler *m_dbHandler;
    TupExportInterface *m_videoExporter;
    TupExportInterface *m_imageExporter;
};

#endif // PROJECTRENDERER_H
