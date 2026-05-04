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

#include "tupexportinterface.h"
#include "databasehandler.h"

class TupProject;
class TupScene;

class ProjectRenderer : public QObject
{
    Q_OBJECT

public:
    struct RenderResult {
        bool success;
        QString mp4Path;
        QString errorMessage;
    };

    explicit ProjectRenderer(DatabaseHandler *dbHandler, QObject *parent = nullptr);

    // Renders all scenes of the given project_id to a MP4 file in the
    // configured render output directory. Updates tupitube_project.last_rendered_at
    // on success. Thread-safe to call but blocks until rendering is complete.
    RenderResult renderProject(int projectId);

private:
    void loadVideoPlugin();
    double calculateDuration(TupProject *project,
                             QList<TupScene *> &outSceneList,
                             int &outThumbScene, int &outThumbFrame);
    bool resizeVideo(const QString &code, const QString &input, const QSize &size);

    DatabaseHandler *m_dbHandler;
    TupExportInterface *m_exporter;
};

#endif // PROJECTRENDERER_H
