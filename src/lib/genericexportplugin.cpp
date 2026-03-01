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
#include "genericexportplugin.h"
#include "tuplayer.h"
#include "tupanimationrenderer.h"
#include "tmoviegeneratorinterface.h"
#include "tffmpegmoviegenerator.h"
#include "tupwatermark.h"

#include <QImage>
#include <QPainter>
#include <QGraphicsTextItem>

#ifdef TUP_DEBUG
    #include <QDebug>
#endif

GenericExportPlugin::GenericExportPlugin()
{
}

GenericExportPlugin::~GenericExportPlugin()
{
}

TupExportInterface::Plugin GenericExportPlugin::key()
{
    return TupExportInterface::VideoFormats;
}

QString GenericExportPlugin::formatName() const
{
    return tr("Video Formats");
}

TupExportInterface::Formats GenericExportPlugin::availableFormats()
{
    return TupExportInterface::PNG | TupExportInterface::JPEG | TupExportInterface::MP4;
}

bool GenericExportPlugin::exportToFormat(int colorAlpha, const QString &filePath, const QList<TupScene *> &scenes,
                                         TupExportInterface::Format format, const QSize &size, const QSize &newSize, int fps,
                                         TupProject *project, bool waterMark)
{
    Q_UNUSED(colorAlpha)
    Q_UNUSED(filePath)
    Q_UNUSED(scenes)
    Q_UNUSED(format)
    Q_UNUSED(size)
    Q_UNUSED(newSize)
    Q_UNUSED(fps)
    Q_UNUSED(project)
    Q_UNUSED(waterMark)
    return false; // Not implemented for server use
}

bool GenericExportPlugin::exportFrame(int frameIndex, const QColor color, const QString &filePath, TupScene *scene,
                                      const QSize &size, TupProject *project, bool waterMark, bool showForegroundView)
{
    Q_UNUSED(frameIndex)
    Q_UNUSED(color)
    Q_UNUSED(filePath)
    Q_UNUSED(scene)
    Q_UNUSED(size)
    Q_UNUSED(project)
    Q_UNUSED(waterMark)
    Q_UNUSED(showForegroundView)
    return false; // Not implemented for server use
}

bool GenericExportPlugin::exportToFormatLocal(const QColor color, const QString &filePath, const QList<TupScene *> &scenes, 
                                         TupExportInterface::Format format, const QSize &size, const QSize &newSize, int fps,
                                         TupLibrary *library, bool waterMark)
{
    Q_UNUSED(color)
    Q_UNUSED(filePath)
    Q_UNUSED(scenes)
    Q_UNUSED(format)
    Q_UNUSED(size)
    Q_UNUSED(newSize)
    Q_UNUSED(fps)
    Q_UNUSED(library)
    Q_UNUSED(waterMark)

    return true;
}

bool GenericExportPlugin::exportFrameLocal(int frameIndex, const QColor color, const QString &path, TupScene *scene,
                                      const QSize &size, TupLibrary *library, bool waterMark)
{
    QString imagePath = path;
    const char *extension;

    if (imagePath.endsWith(".PNG", Qt::CaseInsensitive)) {
        extension = "PNG";
    } else if (imagePath.endsWith(".JPG", Qt::CaseInsensitive) || imagePath.endsWith("JPEG", Qt::CaseInsensitive)) {
        extension = "JPG";
    } else {
        extension = "PNG"; 
        imagePath += ".png";  
    }

    TupAnimationRenderer renderer(library, waterMark);
    renderer.setScene(scene, size, color);

    renderer.renderPhotogram(frameIndex);
    QImage image(size, QImage::Format_RGB32);

    {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        renderer.render(&painter);
    }

    return image.save(imagePath, extension);
}

QString GenericExportPlugin::getExceptionMsg() const 
{
    return errorMsg;
}

bool GenericExportPlugin::exportToAnimatic(const QString &filePath, const QList<QImage> images, const QList<int> indexes,
                                            TupExportInterface::Format format, const QSize &size, int fps)
{
    Q_UNUSED(filePath)
    Q_UNUSED(images)
    Q_UNUSED(indexes)
    Q_UNUSED(format)
    Q_UNUSED(size)
    Q_UNUSED(fps)
    return true;
}
