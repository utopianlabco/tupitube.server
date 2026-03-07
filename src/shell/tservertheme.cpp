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

#include "tservertheme.h"

#include <QFile>
#include <QColor>
#include <QDebug>
#include <QCoreApplication>

QString TServerTheme::themeStyles(int themeType, const QString &bgColor)
{
    QString theme = (themeType == DARK_THEME) ? "dark" : "default";
    
    // Try to load from Qt resources first
    QString themePath = QString(":/themes/%1/config/ui.qss").arg(theme);
    
    #ifdef TUP_DEBUG
        qDebug() << "[TServerTheme::themeStyles()] - Loading ui.qss ->" << themePath;
    #endif

    QString themeStyles = "";
    QFile file(themePath);
    
    if (file.exists() && file.open(QFile::ReadOnly)) {
        themeStyles = QLatin1String(file.readAll());
        file.close();
        
        if (themeStyles.isEmpty()) {
            #ifdef TUP_DEBUG
                qWarning() << "[TServerTheme::themeStyles()] - Warning: Theme file is empty!";
            #endif
            return "";
        }

        // Replace BG_PARAM placeholder with actual background color
        themeStyles = themeStyles.replace("BG_PARAM", bgColor);
        
        // For dark theme, calculate button background color
        if (themeType == DARK_THEME) {
            QColor buttonBgColor(bgColor);
            buttonBgColor.setRed(qMin(buttonBgColor.red() + 30, 255));
            buttonBgColor.setGreen(qMin(buttonBgColor.green() + 30, 255));
            buttonBgColor.setBlue(qMin(buttonBgColor.blue() + 30, 255));
            themeStyles = themeStyles.replace("BUTTON_BG", buttonBgColor.name());
        }
        
        return themeStyles;
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TServerTheme::themeStyles()] - Warning: Theme file doesn't exist ->" << themePath;
        #endif
    }

    return "";
}

QString TServerTheme::defaultBgColor(int themeType)
{
    if (themeType == DARK_THEME)
        return "#353535";
    else
        return "#a0a0a0";  // Matches tupitube.desk light theme
}

QString TServerTheme::themeName(int themeType)
{
    if (themeType == DARK_THEME)
        return "dark";
    else
        return "default";
}
