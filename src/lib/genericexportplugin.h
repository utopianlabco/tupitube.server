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
#ifndef GENERICEXPORTPLUGIN_H
#define GENERICEXPORTPLUGIN_H

#include "tupexportpluginobject.h"
#include "tglobal.h"

#include <QColor>

class GenericExportPlugin : public TupExportPluginObject
{
    Q_OBJECT

    public:
        GenericExportPlugin();
        ~GenericExportPlugin();

        virtual QString formatName() const;
        virtual TupExportInterface::Plugin key();

        TupExportInterface::Formats availableFormats();

        // Interface pure virtual methods (must be implemented)
        bool exportToFormat(int colorAlpha, const QString &filePath, const QList<TupScene *> &scenes, Format format,
                            const QSize &size, const QSize &newSize, int fps, TupProject *project, bool waterMark = false) override;
        bool exportFrame(int frameIndex, const QColor color, const QString &filePath, TupScene *scene, const QSize &size,
                         TupProject *project, bool waterMark = false, bool showForegroundView = true) override;
        bool exportToAnimatic(const QString &filePath, const QList<QImage> images, const QList<int> indexes,
                              TupExportInterface::Format format, const QSize &size, int fps) override;

        // Server-specific methods
        bool exportToFormatLocal(const QColor color, const QString &filePath, const QList<TupScene *> &scenes, Format format,
                            const QSize &size, const QSize &newSize, int fps, TupLibrary *library = nullptr, bool waterMark = true);
        bool exportFrameLocal(int frameIndex, const QColor color, const QString &path, TupScene *scene, const QSize &size, TupLibrary *library = nullptr, 
			 bool waterMark = true);

        QString getExceptionMsg() const;
        QString errorMsg;

    private:
        QString m_baseName;
};

#endif
