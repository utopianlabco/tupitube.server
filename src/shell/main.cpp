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

#include "tapplicationproperties.h"
#include "tconfig.h"
#include "tupserverwindow.h"
#include "tservertheme.h"
#include "logger.h"
#include "settings.h"
#include "databasehandler.h"

#include <QApplication>
#include <QDebug>
#include <QTranslator>
#include <QLocale>
#include <QNetworkInterface>
#include <QTimer>
#include <QStyleFactory>

#include <QFileDialog>
#include <QMessageBox>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <QDir>

static void cleanup(int);
#endif

int main(int argc, char *argv[])
{
    #ifdef Q_OS_UNIX
        QString student = QString::fromLocal8Bit(::getenv("USER"));
        QString uid = QString::fromLocal8Bit(::getenv("UID"));

        if (student.compare("root") == 0 || uid.compare("0") == 0) {
            qDebug() << "[main.cpp] Fatal Error: For security reasons you can't run this service as root student.";
            qDebug() << "[main.cpp] Please, try it as different student (UID != 0).";
            exit(0);
        }
    #endif

    QApplication app(argc, argv);
    app.setApplicationName("tupitube_server");
    
    #ifdef Q_OS_UNIX
        signal(SIGINT, cleanup);
        signal(SIGTERM, cleanup);
        signal(SIGSEGV, cleanup);
    #endif

    TCONFIG->beginGroup("General");
    QString dataPath = TCONFIG->value("DataPath").toString();
    TCONFIG->endGroup();

    QString databasePath = "";
    QString cachePath = "";
    QString projectsPath = "";

    if (dataPath.isEmpty() || !QDir(dataPath).exists()) {
        QString lightBg = TServerTheme::defaultBgColor(LIGHT_THEME);
        QString lightStyle = TServerTheme::themeStyles(LIGHT_THEME, lightBg);

        // Custom dialog for data path selection with light theme
        QDialog pathDialog;
        pathDialog.setWindowTitle("TupiTube Server Data Path");
        pathDialog.resize(500, 120);
        QVBoxLayout *mainLayout = new QVBoxLayout(&pathDialog);

        pathDialog.setStyleSheet(lightStyle);

        QLabel *infoLabel = new QLabel("To run TupiTube Server you need to set a data directory to store all the projects and information related to your students");
        infoLabel->setWordWrap(true);
        mainLayout->addWidget(infoLabel);

        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *pathLabel = new QLabel("Data Path:");
        QLineEdit *pathEdit = new QLineEdit();
        pathEdit->setReadOnly(true);
        QPushButton *browseBtn = new QPushButton;
        browseBtn->setIcon(pathDialog.style()->standardIcon(QStyle::SP_DirOpenIcon));
        browseBtn->setToolTip("Browse for data directory");
        rowLayout->addWidget(pathLabel);
        rowLayout->addWidget(pathEdit);
        rowLayout->addWidget(browseBtn);
        mainLayout->addLayout(rowLayout);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        mainLayout->addWidget(buttonBox);
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

        QObject::connect(browseBtn, &QPushButton::clicked, [&]() {
            QString folder = QFileDialog::getExistingDirectory(&pathDialog, "Select TupiTube Server Data Path", QDir::homePath());
            if (!folder.isEmpty()) {
                pathEdit->setText(folder);
                buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
        });
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &pathDialog, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, &pathDialog, &QDialog::reject);

        if (pathDialog.exec() != QDialog::Accepted || pathEdit->text().isEmpty()) {
            qWarning() << "[main.cpp] The path dialog was cancelled";
            return 0;
        }
        dataPath = pathEdit->text();

        // Auto-detect local IP address
        QString localIP = "0.0.0.0";
        foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
                localIP = address.toString();
                break;
            }
        }

        TCONFIG->beginGroup("Connection");
        TCONFIG->setValue("Host", localIP);
        TCONFIG->setValue("Port", "8080");
        TCONFIG->endGroup();

        TCONFIG->beginGroup("General");
        TCONFIG->setValue("DataPath", dataPath);
        TCONFIG->setValue("LogPath", QDir::tempPath());
        TCONFIG->setValue("Language", QLocale::system().name().left(2));
        TCONFIG->endGroup();

        // Database path: TUPITUBE_DATA_PATH/sqlite
        TCONFIG->beginGroup("Database");
        TCONFIG->setValue("Driver", "QSQLITE");
        TCONFIG->setValue("DbName", "tupitube.db");
        TCONFIG->setValue("DatabasePath", dataPath + "/sqlite");
        TCONFIG->endGroup();
        
        // Cache path: TUPITUBE_DATA_PATH/cache
        TCONFIG->beginGroup("Cache");
        TCONFIG->setValue("CachePath", dataPath + "/cache");
        TCONFIG->endGroup();
        
        // Projects path: TUPITUBE_DATA_PATH/projects
        TCONFIG->beginGroup("Projects");
        TCONFIG->setValue("ProjectsPath", dataPath + "/projects");
        TCONFIG->endGroup();

        // Theme: default to dark theme
        TCONFIG->beginGroup("Theme");
        TCONFIG->setValue("UITheme", LIGHT_THEME);
        TCONFIG->setValue("BgColor", TServerTheme::defaultBgColor(LIGHT_THEME));
        TCONFIG->endGroup();

        TCONFIG->sync();

        #ifdef TUP_DEBUG
            qWarning() << "[main.cpp] Loading current settings...";
            qDebug() << "";
        #endif

        // Load Database path settings
        TCONFIG->beginGroup("Database");
        databasePath = TCONFIG->value("DatabasePath").toString();
        TCONFIG->endGroup();
        
        // Load Cache path settings
        TCONFIG->beginGroup("Cache");
        cachePath = TCONFIG->value("CachePath").toString();
        TCONFIG->endGroup();
        
        // Load Projects path settings
        TCONFIG->beginGroup("Projects");
        projectsPath = TCONFIG->value("ProjectsPath").toString();
        TCONFIG->endGroup();

        // Create database directory if it doesn't exist
        QDir dbDir(databasePath);
        if (!dbDir.exists()) {
            if (!dbDir.mkpath(databasePath)) {
                #ifdef TUP_DEBUG
                    qDebug() << "[main.cpp] Fatal Error: Unable to create database dir: " << databasePath;
                #endif
                exit(1);
            } else {
                #ifdef TUP_DEBUG
                    qWarning() << "[main.cpp] Creating database dir... [ok]";
                #endif
            }
        }

        // Create cache directory if it doesn't exist
        QDir cache(cachePath);
        if (!cache.exists()) {
            if (!cache.mkpath(cachePath)) {
                #ifdef TUP_DEBUG
                    qDebug() << "[main.cpp] Fatal Error: Unable to create cache dir: " << cachePath;
                #endif
                exit(1);
            } else {
                #ifdef TUP_DEBUG
                    qWarning() << "[main.cpp] Creating cache dir... [ok]";
                #endif
            }
        }

        // Clean cache directory at startup
        QDir cacheCleaner(cachePath);
        cacheCleaner.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (QString entry, cacheCleaner.entryList()) {
            QString entryPath = cachePath + "/" + entry;
            QFileInfo info(entryPath);
            if (info.isDir()) {
                QDir(entryPath).removeRecursively();
            } else {
                QFile::remove(entryPath);
            }
        }
        #ifdef TUP_DEBUG
            qWarning() << "[main.cpp] Cache directory cleaned at startup.";
        #endif

        // Create projects directory if it doesn't exist
        QDir projectsDir(projectsPath);
        if (!projectsDir.exists()) {
            if (!projectsDir.mkpath(projectsPath)) {
                #ifdef TUP_DEBUG
                    qDebug() << "[main.cpp] Fatal Error: Unable to create projects dir: " << projectsPath;
                #endif
                exit(1);
            } else {
                #ifdef TUP_DEBUG
                    qWarning() << "[main.cpp] Creating projects dir... [ok]";
                #endif
            }
        }

        DatabaseHandler *m_dbHandler = new DatabaseHandler();
        if (m_dbHandler) {
            m_dbHandler->initDataBase();
            m_dbHandler->createDatabaseSchema();
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[main.cpp] Fatal Error: Unable to initialize database handler.";
            #endif
            exit(1);
        }

        FirstLaunchWizard wizard;
        wizard.setStyleSheet(lightStyle);
        int wizardResult = wizard.exec();
        qDebug() << "[main.cpp] Wizard dialog closed. Result:" << (wizardResult == QDialog::Accepted ? "Accepted" : "Rejected");
        if (wizardResult != QDialog::Accepted) {
            qDebug() << "[main.cpp] Wizard was canceled or not finished. Exiting app.";
            return 0;
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[main.cpp] Wizard completed successfully.";
            #endif
        }

        // Print wizard information to the console
        qDebug() << "[main.cpp] Wizard Data:";
        qDebug() << "  Class Name:" << wizard.field("className").toString();
        qDebug() << "  Class Year:" << wizard.field("classYear").toInt();
        qDebug() << "  Class Description:" << wizard.field("classDesc").toString();
        qDebug() << "  Period Name:" << wizard.field("periodName").toString();
        qDebug() << "  Period Year:" << wizard.field("periodYear").toInt();
        qDebug() << "  Period Start Date:" << wizard.field("periodStartDate").toDate();
        qDebug() << "  Period End Date:" << wizard.field("periodEndDate").toDate();
        qDebug() << "  Student Username:" << wizard.field("studentUsername").toString();
        qDebug() << "  Student Full Name:" << wizard.field("studentFullName").toString();
        qDebug() << "  Student Password:" << wizard.field("studentPassword").toString();
        qDebug() << "  Student Is Creator:" << wizard.field("studentIsCreator").toBool();
        qDebug() << "  Student Class:" << wizard.field("studentClass").toString();

        // Save wizard information to the database
        QString className = wizard.field("className").toString();
        int classYear = wizard.field("classYear").toInt();
        QString classDesc = wizard.field("classDesc").toString();
        QString periodName = wizard.field("periodName").toString();
        int periodYear = wizard.field("periodYear").toInt();
        QDate periodStartDate = wizard.field("periodStartDate").toDate();
        QDate periodEndDate = wizard.field("periodEndDate").toDate();
        QString studentUsername = wizard.field("studentUsername").toString();
        QString studentFullName = wizard.field("studentFullName").toString();
        QString studentPassword = wizard.field("studentPassword").toString();
        bool studentIsCreator = wizard.field("studentIsCreator").toBool();
        QString studentClass = wizard.field("studentClass").toString();

        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData(studentPassword.toUtf8());
        QString hashedPassword = md5.result().toHex();

        bool classOk = m_dbHandler->addClass(className, classYear, classDesc);
        bool periodOk = m_dbHandler->addPeriod(periodName, periodYear, periodStartDate.toString("yyyy-MM-dd"), periodEndDate.toString("yyyy-MM-dd"));
        int newClassId = m_dbHandler->getClassIdByName(className);
        bool studentOk = (newClassId > 0) && m_dbHandler->addStudent(studentUsername, studentFullName, hashedPassword, true, studentIsCreator, newClassId);

        qDebug() << "[main.cpp] addClass result:" << classOk;
        qDebug() << "[main.cpp] addPeriod result:" << periodOk;
        qDebug() << "[main.cpp] addStudent result:" << studentOk;
    }

    #ifdef TUP_DEBUG
        qWarning() << "[main.cpp] Loading current settings...";
        qDebug() << "";
    #endif

    // Load Database path settings
    TCONFIG->beginGroup("Database");
    databasePath = TCONFIG->value("DatabasePath").toString();
    TCONFIG->endGroup();
    
    // Load Cache path settings
    TCONFIG->beginGroup("Cache");
    cachePath = TCONFIG->value("CachePath").toString();
    TCONFIG->endGroup();
    
    // Load Projects path settings
    TCONFIG->beginGroup("Projects");
    projectsPath = TCONFIG->value("ProjectsPath").toString();
    TCONFIG->endGroup();

    #ifdef TUP_DEBUG
        qWarning() << "[main.cpp] Data path:" << dataPath;
    #endif

    QString pluginDir = QString::fromLocal8Bit(::getenv("TUPITUBE_PLUGIN"));

    if (pluginDir.isEmpty()) {
        QString serverHome = QString::fromLocal8Bit(::getenv("TUPITUBE_SERVER_HOME"));

        if (!serverHome.isEmpty()) {
            #ifdef Q_OS_WIN
                pluginDir = serverHome + "/plugins";
            #else
                pluginDir = serverHome + "/lib/tupitube/plugins";
            #endif
        } else {
            #ifdef Q_OS_WIN
                pluginDir = QCoreApplication::applicationDirPath() + "/plugins";
            #else
                QString tupitubeHome = QString::fromLocal8Bit(::getenv("TUPITUBE_HOME"));
                if (!tupitubeHome.isEmpty())
                    pluginDir = tupitubeHome + "/lib/tupitube/plugins";
                else
                    pluginDir = "/usr/lib/tupitube/plugins";
            #endif
        }
    }

    kAppProp->setPluginDir(pluginDir);
    kAppProp->setRepositoryDir(projectsPath);
    kAppProp->setCacheDir(cachePath);

    // Load translations from TUPITUBE_SERVER_HOME/data/translations
    QString translationsPath = QString::fromLocal8Bit(::getenv("TUPITUBE_SERVER_HOME"));
    if (translationsPath.isEmpty()) {
        // Fallback: use relative path from bin directory
        #ifdef Q_OS_WIN
            translationsPath = QCoreApplication::applicationDirPath() + "/data/translations";
        #else
            translationsPath = QCoreApplication::applicationDirPath() + "/../src/shell/data/translations";
        #endif
    } else {
        translationsPath += "/data/translations";
    }
    
    TCONFIG->beginGroup("General");
    QString language = TCONFIG->value("Language", QLocale::system().name().left(2)).toString();
    TCONFIG->endGroup();
    
    #ifdef TUP_DEBUG
        qWarning() << "[main.cpp] Language:" << language;
    #endif
    
    // Skip translation loading for English (source code is already in English)
    if (language != "en") {
        #ifdef TUP_DEBUG
            qWarning() << "[main.cpp] TranslationsPath:" << translationsPath;
        #endif
        
        QTranslator *translator = new QTranslator(&app);
        QString translationFile = translationsPath + "/tupitube_server_" + language + ".qm";
        if (translator->load(translationFile)) {
            app.installTranslator(translator);
            #ifdef TUP_DEBUG
                qWarning() << "[main.cpp] Loaded translation:" << translationFile;
            #endif
        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[main.cpp] No translation found at:" << translationFile;
            #endif
            delete translator;
        }
    }

    // Set Fusion style for consistent cross-platform appearance
    app.setStyle(QStyleFactory::create("Fusion"));

    // Load theme from settings
    TCONFIG->beginGroup("Theme");
    int uiTheme = TCONFIG->value("UITheme", DARK_THEME).toInt();
    QString bgColor = TCONFIG->value("BgColor", TServerTheme::defaultBgColor(uiTheme)).toString();
    TCONFIG->endGroup();

    // Apply QSS stylesheet
    QString styleSheet = TServerTheme::themeStyles(uiTheme, bgColor);
    if (!styleSheet.isEmpty()) {
        app.setStyleSheet(styleSheet);
    }

    TupServerWindow window;
    window.show();
    // Auto-start the server
    QTimer::singleShot(100, &window, &TupServerWindow::startServer);
    return app.exec();
}

#ifdef Q_OS_UNIX

void cleanup(int s)
{
    #ifdef TUP_DEBUG
           qWarning() << "[main.cpp] Finishing with signal: " << s;
    #endif
    
    // QApplication::flush();
    QCoreApplication::processEvents();
    QApplication::exit(0);
    
    Logger::self()->info(QObject::tr("[main.cpp] Closing server with signal: ") + QString::number(s));
    
    if (s == 11) {
        #ifdef TUP_DEBUG
            qDebug() << "[main.cpp] System has crashed! Restarting...";
        #endif
        int pid = ::getpid();
        QString path = QString::fromLocal8Bit(::getenv("TUPITUBE_SERVER_HOME")) + "/bin/tupitube.server &";
        QByteArray ba = path.toUtf8();
        int result = system(ba.data());

        #ifdef TUP_DEBUG
            qDebug() << "[main.cpp] Result: " << result;
        #endif

        kill(pid, 9);
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[main.cpp] Ctrl+C or Unknown epic crash signal: " << s;
        #endif
    }
}

#endif
