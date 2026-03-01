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
#include "tupserverwindow.h"
#include "tconfig.h"
#include "logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QApplication>
#include <QStyle>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QNetworkInterface>
#include <QCryptographicHash>

static QStringList getLocalIPAddresses()
{
    QStringList addresses;
    addresses << "0.0.0.0";  // Listen on all interfaces
    
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        // Skip loopback and inactive interfaces
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;
        if (!(interface.flags() & QNetworkInterface::IsUp))
            continue;
        if (!(interface.flags() & QNetworkInterface::IsRunning))
            continue;
            
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
            QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                QString ip = address.toString();
                QString label = QString("%1 (%2)").arg(ip, interface.humanReadableName());
                addresses << label;
            }
        }
    }
    
    return addresses;
}

TupServerWindow::TupServerWindow(QWidget *parent) : QMainWindow(parent),
    m_server(nullptr), m_serverRunning(false), m_dbHandler(nullptr)
{
    setWindowTitle(tr("TupiTube Server"));
    setWindowIcon(QIcon(":/icons/server.png"));
    setMinimumSize(700, 500);

    m_server = new TcpServer(this);
    m_dbHandler = new DatabaseHandler();

    // Connect server signals
    connect(m_server, &TcpServer::connectionCountChanged, this, &TupServerWindow::onConnectionCountChanged);
    connect(m_server, &TcpServer::userConnected, this, &TupServerWindow::onUserConnected);
    connect(m_server, &TcpServer::userDisconnected, this, &TupServerWindow::onUserDisconnected);
    connect(m_server, &TcpServer::logMessage, this, &TupServerWindow::onLogMessage);

    setupUI();
    setupMenuBar();
    setupTrayIcon();
    loadSettings();

    // Update uptime every second
    m_uptimeTimer = new QTimer(this);
    connect(m_uptimeTimer, &QTimer::timeout, this, [this]() {
        if (m_serverRunning && m_startTime.isValid()) {
            qint64 secs = m_startTime.secsTo(QDateTime::currentDateTime());
            int hours = secs / 3600;
            int mins = (secs % 3600) / 60;
            int seconds = secs % 60;
            m_uptimeLabel->setText(QString("%1:%2:%3")
                .arg(hours, 2, 10, QChar('0'))
                .arg(mins, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0')));
        }
    });
    m_uptimeTimer->start(1000);

    appendLog(tr("TupiTube Server GUI initialized"), "INFO");
}

TupServerWindow::~TupServerWindow()
{
    if (m_serverRunning) {
        m_server->close();
    }
    if (m_dbHandler) {
        delete m_dbHandler;
        m_dbHandler = nullptr;
    }
}

void TupServerWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    setupTabs();
    mainLayout->addWidget(m_tabWidget);
}

void TupServerWindow::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setMenuRole(QAction::QuitRole);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // Server menu
    QMenu *serverMenu = menuBar()->addMenu(tr("&Server"));

    m_toggleServerAction = serverMenu->addAction(tr("&Start Server"));
    m_toggleServerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(m_toggleServerAction, &QAction::triggered, this, &TupServerWindow::toggleServer);

    serverMenu->addSeparator();

    QAction *clearLogsAction = serverMenu->addAction(tr("&Clear Logs"));
    clearLogsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(clearLogsAction, &QAction::triggered, this, &TupServerWindow::clearLogs);

    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    QAction *statusTabAction = viewMenu->addAction(tr("&Status"));
    statusTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    connect(statusTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(0);
    });

    QAction *usersTabAction = viewMenu->addAction(tr("&Users"));
    usersTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
    connect(usersTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(1);
    });

    QAction *logsTabAction = viewMenu->addAction(tr("&Logs"));
    logsTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
    connect(logsTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(2);
    });

    QAction *settingsTabAction = viewMenu->addAction(tr("Se&ttings"));
    settingsTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));
    connect(settingsTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(3);
    });

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    aboutAction->setMenuRole(QAction::AboutRole);
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About TupiTube Server"),
            tr("<h3>TupiTube Server</h3>"
               "<p>A collaboration server for TupiTube Desk.</p>"
               "<p>Allows multiple artists to work on the same animation project in real-time.</p>"
               "<p>Website: <a href='http://www.tupitube.com'>www.tupitube.com</a></p>"));
    });
}

void TupServerWindow::setupTabs()
{
    m_tabWidget = new QTabWidget(this);

    m_statusTab = new QWidget();
    m_usersTab = new QWidget();
    m_logsTab = new QWidget();
    m_settingsTab = new QWidget();

    setupStatusTab();
    setupUsersTab();
    setupLogsTab();
    setupSettingsTab();

    m_tabWidget->addTab(m_statusTab, QIcon(":/icons/status.png"), tr("Status"));
    m_tabWidget->addTab(m_usersTab, QIcon(":/icons/users.png"), tr("Users"));
    m_tabWidget->addTab(m_logsTab, QIcon(":/icons/logs.png"), tr("Logs"));
    m_tabWidget->addTab(m_settingsTab, QIcon(":/icons/settings.png"), tr("Settings"));
}

void TupServerWindow::setupStatusTab()
{
    QVBoxLayout *layout = new QVBoxLayout(m_statusTab);
    layout->setSpacing(20);

    // Server control group
    QGroupBox *controlGroup = new QGroupBox(tr("Server Control"));
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);

    m_toggleButton = new QPushButton(tr("Start Server"));
    m_toggleButton->setMinimumHeight(50);
    m_toggleButton->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; }");
    connect(m_toggleButton, &QPushButton::clicked, this, &TupServerWindow::toggleServer);
    controlLayout->addWidget(m_toggleButton);

    layout->addWidget(controlGroup);

    // Status info group
    QGroupBox *infoGroup = new QGroupBox(tr("Server Information"));
    QFormLayout *infoLayout = new QFormLayout(infoGroup);
    infoLayout->setSpacing(10);

    m_statusLabel = new QLabel(tr("Stopped"));
    m_statusLabel->setStyleSheet("QLabel { color: #c0392b; font-weight: bold; font-size: 14px; }");
    infoLayout->addRow(tr("Status:"), m_statusLabel);

    m_hostLabel = new QLabel("-");
    infoLayout->addRow(tr("Host:"), m_hostLabel);

    m_portLabel = new QLabel("-");
    infoLayout->addRow(tr("Port:"), m_portLabel);

    m_connectionCountLabel = new QLabel("0");
    m_connectionCountLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    infoLayout->addRow(tr("Active Connections:"), m_connectionCountLabel);

    m_uptimeLabel = new QLabel("00:00:00");
    infoLayout->addRow(tr("Uptime:"), m_uptimeLabel);

    layout->addWidget(infoGroup);
    layout->addStretch();
}

void TupServerWindow::setupUsersTab()
{
    QVBoxLayout *layout = new QVBoxLayout(m_usersTab);

    // Connected Users Section
    QGroupBox *connectedGroup = new QGroupBox(tr("Connected Users"));
    QVBoxLayout *connectedLayout = new QVBoxLayout(connectedGroup);

    m_connectedUsersTable = new QTableWidget(0, 4);
    m_connectedUsersTable->setHorizontalHeaderLabels({tr("Username"), tr("IP Address"), tr("Connected At"), tr("Status")});
    m_connectedUsersTable->horizontalHeader()->setStretchLastSection(true);
    m_connectedUsersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_connectedUsersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_connectedUsersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_connectedUsersTable->setAlternatingRowColors(true);
    m_connectedUsersTable->setMaximumHeight(150);

    connectedLayout->addWidget(m_connectedUsersTable);
    layout->addWidget(connectedGroup);

    // Registered Users Section
    QGroupBox *registeredGroup = new QGroupBox(tr("Registered Users"));
    QVBoxLayout *registeredLayout = new QVBoxLayout(registeredGroup);

    m_registeredUsersTable = new QTableWidget(0, 5);
    m_registeredUsersTable->setHorizontalHeaderLabels({tr("ID"), tr("Username"), tr("Name"), tr("Enabled"), tr("Creator")});
    m_registeredUsersTable->horizontalHeader()->setStretchLastSection(true);
    m_registeredUsersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_registeredUsersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_registeredUsersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_registeredUsersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_registeredUsersTable->setAlternatingRowColors(true);
    m_registeredUsersTable->setColumnHidden(0, true); // Hide ID column

    registeredLayout->addWidget(m_registeredUsersTable);

    // Buttons for user management
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_addUserButton = new QPushButton(tr("Add User"));
    m_addUserButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    connect(m_addUserButton, &QPushButton::clicked, this, &TupServerWindow::addUser);
    buttonLayout->addWidget(m_addUserButton);

    m_editUserButton = new QPushButton(tr("Edit User"));
    m_editUserButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(m_editUserButton, &QPushButton::clicked, this, &TupServerWindow::editUser);
    buttonLayout->addWidget(m_editUserButton);

    m_removeUserButton = new QPushButton(tr("Remove User"));
    m_removeUserButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    connect(m_removeUserButton, &QPushButton::clicked, this, &TupServerWindow::removeUser);
    buttonLayout->addWidget(m_removeUserButton);

    buttonLayout->addStretch();

    m_refreshUsersButton = new QPushButton(tr("Refresh"));
    m_refreshUsersButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_refreshUsersButton, &QPushButton::clicked, this, &TupServerWindow::refreshUsersList);
    buttonLayout->addWidget(m_refreshUsersButton);

    registeredLayout->addLayout(buttonLayout);
    layout->addWidget(registeredGroup);

    // Note: User list will be loaded when server starts (database must be open first)
}

void TupServerWindow::setupLogsTab()
{
    QVBoxLayout *layout = new QVBoxLayout(m_logsTab);

    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setStyleSheet("QTextEdit { font-family: 'Courier New', monospace; font-size: 12px; }");
    layout->addWidget(m_logView);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_clearLogsButton = new QPushButton(tr("Clear Logs"));
    connect(m_clearLogsButton, &QPushButton::clicked, this, &TupServerWindow::clearLogs);
    buttonLayout->addWidget(m_clearLogsButton);

    layout->addLayout(buttonLayout);
}

void TupServerWindow::setupSettingsTab()
{
    QVBoxLayout *layout = new QVBoxLayout(m_settingsTab);

    // Connection settings
    QGroupBox *connectionGroup = new QGroupBox(tr("Connection Settings"));
    QFormLayout *connectionLayout = new QFormLayout(connectionGroup);

    m_hostCombo = new QComboBox();
    QStringList addresses = getLocalIPAddresses();
    m_hostCombo->addItems(addresses);
    connectionLayout->addRow(tr("Host:"), m_hostCombo);

    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(8080);
    connectionLayout->addRow(tr("Port:"), m_portSpin);

    layout->addWidget(connectionGroup);

    // Storage settings - all paths derived from a single Data Path
    QGroupBox *storageGroup = new QGroupBox(tr("Storage Settings"));
    QFormLayout *storageLayout = new QFormLayout(storageGroup);

    // Data Path - the base directory for all resources
    QHBoxLayout *dataPathLayout = new QHBoxLayout();
    m_dataPathEdit = new QLineEdit();
    m_dataPathEdit->setPlaceholderText(QDir::homePath() + "/.tupitube_server");
    dataPathLayout->addWidget(m_dataPathEdit);
    m_browseDataPathButton = new QPushButton(tr("Browse..."));
    connect(m_browseDataPathButton, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Data Directory"),
                                                         m_dataPathEdit->text());
        if (!dir.isEmpty()) {
            m_dataPathEdit->setText(dir);
            updateDerivedPaths();
        }
    });
    dataPathLayout->addWidget(m_browseDataPathButton);
    storageLayout->addRow(tr("Data Path:"), dataPathLayout);

    // Update derived paths when data path changes
    connect(m_dataPathEdit, &QLineEdit::textChanged, this, &TupServerWindow::updateDerivedPaths);

    // Derived paths (read-only info labels)
    m_databasePathLabel = new QLabel();
    m_databasePathLabel->setStyleSheet("color: gray;");
    storageLayout->addRow(tr("Database:"), m_databasePathLabel);

    m_cachePathLabel = new QLabel();
    m_cachePathLabel->setStyleSheet("color: gray;");
    storageLayout->addRow(tr("Cache:"), m_cachePathLabel);

    m_projectsPathLabel = new QLabel();
    m_projectsPathLabel->setStyleSheet("color: gray;");
    storageLayout->addRow(tr("Projects:"), m_projectsPathLabel);

    layout->addWidget(storageGroup);

    // Language settings
    QGroupBox *languageGroup = new QGroupBox(tr("Language Settings"));
    QFormLayout *languageLayout = new QFormLayout(languageGroup);

    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem("Español", "es");
    languageLayout->addRow(tr("Language:"), m_languageCombo);

    layout->addWidget(languageGroup);

    // Save button
    QHBoxLayout *saveLayout = new QHBoxLayout();
    saveLayout->addStretch();
    m_saveSettingsButton = new QPushButton(tr("Save Settings"));
    m_saveSettingsButton->setMinimumWidth(120);
    connect(m_saveSettingsButton, &QPushButton::clicked, this, &TupServerWindow::saveSettings);
    saveLayout->addWidget(m_saveSettingsButton);

    layout->addLayout(saveLayout);
    layout->addStretch();
}

void TupServerWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/server.png"));

    m_trayMenu = new QMenu(this);

    QAction *showAction = m_trayMenu->addAction(tr("Show Window"));
    connect(showAction, &QAction::triggered, this, &QWidget::showNormal);

    QAction *toggleAction = m_trayMenu->addAction(tr("Start Server"));
    connect(toggleAction, &QAction::triggered, this, &TupServerWindow::toggleServer);

    m_trayMenu->addSeparator();

    QAction *quitAction = m_trayMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TupServerWindow::trayIconActivated);

    m_trayIcon->show();
    updateTrayTooltip();
}

void TupServerWindow::loadSettings()
{
    TCONFIG->beginGroup("Connection");
    QString savedHost = TCONFIG->value("Host", "0.0.0.0").toString();
    // Find matching item in the combo
    int index = 0;  // Default to first item (0.0.0.0)
    for (int i = 0; i < m_hostCombo->count(); i++) {
        if (m_hostCombo->itemText(i).startsWith(savedHost)) {
            index = i;
            break;
        }
    }
    m_hostCombo->setCurrentIndex(index);
    m_portSpin->setValue(TCONFIG->value("Port", 8080).toInt());
    TCONFIG->endGroup();

    // Load Data Path
    TCONFIG->beginGroup("General");
    QString defaultDataPath = QDir::homePath() + "/.tupitube_server";
    m_dataPathEdit->setText(TCONFIG->value("DataPath", defaultDataPath).toString());
    QString language = TCONFIG->value("Language", "en").toString();
    int langIndex = m_languageCombo->findData(language);
    if (langIndex >= 0)
        m_languageCombo->setCurrentIndex(langIndex);
    TCONFIG->endGroup();

    // Update derived path labels
    updateDerivedPaths();

    // Update display labels
    QString displayHost = m_hostCombo->currentText();
    if (displayHost.contains(" ("))
        displayHost = displayHost.section(" (", 0, 0);
    m_hostLabel->setText(displayHost);
    m_portLabel->setText(QString::number(m_portSpin->value()));
}

void TupServerWindow::updateDerivedPaths()
{
    QString dataPath = m_dataPathEdit->text();
    m_databasePathLabel->setText(dataPath + "/sqlite");
    m_cachePathLabel->setText(dataPath + "/cache");
    m_projectsPathLabel->setText(dataPath + "/projects");
}

void TupServerWindow::saveSettings()
{
    if (m_serverRunning) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please stop the server before changing settings."));
        return;
    }

    TCONFIG->beginGroup("Connection");
    // Extract just the IP address (remove interface name if present)
    QString hostText = m_hostCombo->currentText();
    if (hostText.contains(" ("))
        hostText = hostText.section(" (", 0, 0);
    TCONFIG->setValue("Host", hostText);
    TCONFIG->setValue("Port", m_portSpin->value());
    TCONFIG->endGroup();

    // Save Data Path and derive all other paths from it
    QString dataPath = m_dataPathEdit->text();

    TCONFIG->beginGroup("General");
    TCONFIG->setValue("DataPath", dataPath);
    QString newLanguage = m_languageCombo->currentData().toString();
    QString oldLanguage = TCONFIG->value("Language", "en").toString();
    TCONFIG->setValue("Language", newLanguage);
    TCONFIG->endGroup();

    TCONFIG->beginGroup("Database");
    TCONFIG->setValue("DatabasePath", dataPath + "/sqlite");
    TCONFIG->endGroup();

    TCONFIG->beginGroup("Cache");
    TCONFIG->setValue("CachePath", dataPath + "/cache");
    TCONFIG->endGroup();

    TCONFIG->beginGroup("Projects");
    TCONFIG->setValue("ProjectsPath", dataPath + "/projects");
    TCONFIG->endGroup();

    TCONFIG->sync();

    // Update display labels
    m_hostLabel->setText(hostText);
    m_portLabel->setText(QString::number(m_portSpin->value()));

    QString message = tr("Settings have been saved.");
    if (newLanguage != oldLanguage) {
        message += "\n\n" + tr("Language change will take effect after restarting the application.");
    } else {
        message += " " + tr("They will take effect when the server is restarted.");
    }

    appendLog(tr("Settings saved successfully"), "INFO");
    QMessageBox::information(this, tr("Settings Saved"), message);
}

void TupServerWindow::toggleServer()
{
    if (m_serverRunning) {
        // Stop server
        m_server->close();
        onServerStopped();
    } else {
        startServer();
    }
}

void TupServerWindow::startServer()
{
    if (m_serverRunning)
        return;
        
    // Start server
    QString host = m_hostCombo->currentText();
    // Extract just the IP address (remove interface name if present)
    if (host.contains(" ("))
        host = host.section(" (", 0, 0);
    int port = m_portSpin->value();

    // Create all data directories from the base path
    QString dataPath = m_dataPathEdit->text();

    // Create database directory if needed
    QString dbPath = dataPath + "/sqlite";
    QDir dbDir(dbPath);
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(dbPath)) {
            appendLog(tr("Failed to create database directory: %1").arg(dbPath), "ERROR");
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to create database directory: %1").arg(dbPath));
            return;
        }
    }

    // Create cache directory if needed
    QString cachePath = dataPath + "/cache";
    QDir cache(cachePath);
    if (!cache.exists()) {
        cache.mkpath(cachePath);
    }

    // Create projects directory if needed
    QString projectsPath = dataPath + "/projects";
    QDir projectsDir(projectsPath);
    if (!projectsDir.exists()) {
        if (!projectsDir.mkpath(projectsPath)) {
            appendLog(tr("Failed to create projects directory: %1").arg(projectsPath), "ERROR");
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to create projects directory: %1").arg(projectsPath));
            return;
        }
    }

    if (m_server->openConnection(host, port)) {
        onServerStarted();
    } else {
        appendLog(tr("Failed to start server on %1:%2").arg(host).arg(port), "ERROR");
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to start server on %1:%2\n\nPlease check if the port is already in use.")
                .arg(host).arg(port));
    }
}

void TupServerWindow::onServerStarted()
{
    m_serverRunning = true;
    m_startTime = QDateTime::currentDateTime();

    m_toggleButton->setText(tr("Stop Server"));
    m_toggleButton->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; background-color: #e74c3c; color: white; }");

    m_statusLabel->setText(tr("Running"));
    m_statusLabel->setStyleSheet("QLabel { color: #27ae60; font-weight: bold; font-size: 14px; }");

    QString displayHost = m_hostCombo->currentText();
    if (displayHost.contains(" ("))
        displayHost = displayHost.section(" (", 0, 0);
    m_hostLabel->setText(displayHost);
    m_portLabel->setText(QString::number(m_portSpin->value()));

    // Update tray menu
    QList<QAction*> actions = m_trayMenu->actions();
    if (actions.size() > 1) {
        actions[1]->setText(tr("Stop Server"));
    }

    // Update menu bar action
    m_toggleServerAction->setText(tr("Sto&p Server"));

    updateTrayTooltip();
    appendLog(tr("Server started on %1:%2").arg(displayHost).arg(m_portSpin->value()), "INFO");

    m_trayIcon->showMessage(tr("TupiTube Server"),
        tr("Server started successfully"), QSystemTrayIcon::Information, 3000);

    // Load registered users now that database is open
    refreshUsersList();
}

void TupServerWindow::onServerStopped()
{
    m_serverRunning = false;

    m_toggleButton->setText(tr("Start Server"));
    m_toggleButton->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; }");

    m_statusLabel->setText(tr("Stopped"));
    m_statusLabel->setStyleSheet("QLabel { color: #c0392b; font-weight: bold; font-size: 14px; }");

    m_connectionCountLabel->setText("0");
    m_uptimeLabel->setText("00:00:00");

    // Clear connected users table
    m_connectedUsersTable->setRowCount(0);

    // Update tray menu
    QList<QAction*> actions = m_trayMenu->actions();
    if (actions.size() > 1) {
        actions[1]->setText(tr("Start Server"));
    }

    // Update menu bar action
    m_toggleServerAction->setText(tr("&Start Server"));

    updateTrayTooltip();
    appendLog(tr("Server stopped"), "INFO");
}

void TupServerWindow::onConnectionCountChanged(int count)
{
    m_connectionCountLabel->setText(QString::number(count));
    updateTrayTooltip();
}

void TupServerWindow::onUserConnected(const QString &username, const QString &ip)
{
    int row = m_connectedUsersTable->rowCount();
    m_connectedUsersTable->insertRow(row);

    m_connectedUsersTable->setItem(row, 0, new QTableWidgetItem(username));
    m_connectedUsersTable->setItem(row, 1, new QTableWidgetItem(ip));
    m_connectedUsersTable->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    m_connectedUsersTable->setItem(row, 3, new QTableWidgetItem(tr("Connected")));

    appendLog(tr("User connected: %1 from %2").arg(username).arg(ip), "INFO");
}

void TupServerWindow::onUserDisconnected(const QString &username)
{
    // Find and remove user from table
    for (int row = 0; row < m_connectedUsersTable->rowCount(); ++row) {
        QTableWidgetItem *item = m_connectedUsersTable->item(row, 0);
        if (item && item->text() == username) {
            m_connectedUsersTable->removeRow(row);
            break;
        }
    }

    appendLog(tr("User disconnected: %1").arg(username), "INFO");
}

void TupServerWindow::onLogMessage(const QString &message, const QString &level)
{
    appendLog(message, level);
}

void TupServerWindow::appendLog(const QString &message, const QString &level)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString color = "#333333";

    if (level == "ERROR" || level == "FATAL") {
        color = "#c0392b";
    } else if (level == "WARN") {
        color = "#f39c12";
    } else if (level == "INFO") {
        color = "#2980b9";
    }

    QString formattedMessage = QString("<span style='color: #666666;'>[%1]</span> "
                                       "<span style='color: %2; font-weight: bold;'>[%3]</span> "
                                       "<span style='color: #333333;'>%4</span>")
                                   .arg(timestamp)
                                   .arg(color)
                                   .arg(level)
                                   .arg(message);

    m_logView->append(formattedMessage);

    // Also log to file via Logger
    if (level == "INFO") {
        Logger::self()->info(message);
    } else if (level == "WARN") {
        Logger::self()->warn(message);
    } else if (level == "ERROR") {
        Logger::self()->error(message);
    } else if (level == "FATAL") {
        Logger::self()->fatal(message);
    }
}

void TupServerWindow::clearLogs()
{
    m_logView->clear();
    appendLog(tr("Logs cleared"), "INFO");
}

void TupServerWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            activateWindow();
        }
    }
}

void TupServerWindow::updateTrayTooltip()
{
    QString status = m_serverRunning ? tr("Running") : tr("Stopped");
    QString connections = m_serverRunning ? QString::number(m_connectedUsersTable->rowCount()) : "0";
    m_trayIcon->setToolTip(tr("TupiTube Server\nStatus: %1\nConnections: %2")
                               .arg(status)
                               .arg(connections));
}

void TupServerWindow::closeEvent(QCloseEvent *event)
{
    QString message;
    if (m_serverRunning)
        message = tr("Server is running. Are you sure you want to quit?");
    else
        message = tr("Are you sure you want to quit?");

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Confirm Exit"),
        message, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) {
        event->ignore();
        return;
    }

    if (m_serverRunning)
        m_server->close();

    event->accept();
    qApp->quit();
}

void TupServerWindow::refreshUsersList()
{
    m_registeredUsersTable->setRowCount(0);

    if (!m_dbHandler) {
        appendLog(tr("Database handler not initialized"), "ERROR");
        return;
    }

    QList<DatabaseHandler::UserInfo> users = m_dbHandler->getAllUsers();

    for (const DatabaseHandler::UserInfo &user : users) {
        int row = m_registeredUsersTable->rowCount();
        m_registeredUsersTable->insertRow(row);

        m_registeredUsersTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.userId)));
        m_registeredUsersTable->setItem(row, 1, new QTableWidgetItem(user.username));
        m_registeredUsersTable->setItem(row, 2, new QTableWidgetItem(user.name));
        m_registeredUsersTable->setItem(row, 3, new QTableWidgetItem(user.isEnabled ? tr("Yes") : tr("No")));
        m_registeredUsersTable->setItem(row, 4, new QTableWidgetItem(user.isCreator ? tr("Yes") : tr("No")));
    }

    appendLog(tr("User list refreshed: %1 users found").arg(users.count()), "INFO");
}

void TupServerWindow::addUser()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add New User"));
    dialog.setMinimumWidth(350);

    QFormLayout *formLayout = new QFormLayout(&dialog);

    QLineEdit *usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText(tr("Enter username"));
    formLayout->addRow(tr("Username:"), usernameEdit);

    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(tr("Enter full name"));
    formLayout->addRow(tr("Full Name:"), nameEdit);

    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Enter password"));
    formLayout->addRow(tr("Password:"), passwordEdit);

    QLineEdit *confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText(tr("Confirm password"));
    formLayout->addRow(tr("Confirm Password:"), confirmPasswordEdit);

    QCheckBox *enabledCheck = new QCheckBox(tr("Account enabled"));
    enabledCheck->setChecked(true);
    formLayout->addRow("", enabledCheck);

    QCheckBox *creatorCheck = new QCheckBox(tr("Can create projects"));
    creatorCheck->setChecked(true);
    formLayout->addRow("", creatorCheck);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    formLayout->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QString username = usernameEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        QString password = passwordEdit->text();
        QString confirmPassword = confirmPasswordEdit->text();

        if (username.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Username cannot be empty"));
            return;
        }

        if (password.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Password cannot be empty"));
            return;
        }

        if (password != confirmPassword) {
            QMessageBox::warning(this, tr("Error"), tr("Passwords do not match"));
            return;
        }

        if (m_dbHandler->usernameExists(username)) {
            QMessageBox::warning(this, tr("Error"), tr("Username already exists"));
            return;
        }

        // Hash the password with MD5 to match client-side hashing
        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData(password.toUtf8());
        QString hashedPassword = md5.result().toHex();

        bool success = m_dbHandler->addUser(username, name, hashedPassword, enabledCheck->isChecked(), creatorCheck->isChecked());

        if (success) {
            appendLog(tr("User '%1' added successfully").arg(username), "INFO");
            refreshUsersList();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to add user"));
            appendLog(tr("Failed to add user '%1'").arg(username), "ERROR");
        }
    }
}

void TupServerWindow::editUser()
{
    int currentRow = m_registeredUsersTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, tr("Information"), tr("Please select a user to edit"));
        return;
    }

    int userId = m_registeredUsersTable->item(currentRow, 0)->text().toInt();
    QString currentUsername = m_registeredUsersTable->item(currentRow, 1)->text();
    QString currentName = m_registeredUsersTable->item(currentRow, 2)->text();
    bool currentEnabled = m_registeredUsersTable->item(currentRow, 3)->text() == tr("Yes");
    bool currentCreator = m_registeredUsersTable->item(currentRow, 4)->text() == tr("Yes");

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit User: %1").arg(currentUsername));
    dialog.setMinimumWidth(350);

    QFormLayout *formLayout = new QFormLayout(&dialog);

    QLineEdit *usernameEdit = new QLineEdit(currentUsername);
    formLayout->addRow(tr("Username:"), usernameEdit);

    QLineEdit *nameEdit = new QLineEdit(currentName);
    formLayout->addRow(tr("Full Name:"), nameEdit);

    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Leave empty to keep current"));
    formLayout->addRow(tr("New Password:"), passwordEdit);

    QLineEdit *confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText(tr("Confirm new password"));
    formLayout->addRow(tr("Confirm Password:"), confirmPasswordEdit);

    QCheckBox *enabledCheck = new QCheckBox(tr("Account enabled"));
    enabledCheck->setChecked(currentEnabled);
    formLayout->addRow("", enabledCheck);

    QCheckBox *creatorCheck = new QCheckBox(tr("Can create projects"));
    creatorCheck->setChecked(currentCreator);
    formLayout->addRow("", creatorCheck);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    formLayout->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QString username = usernameEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        QString password = passwordEdit->text();
        QString confirmPassword = confirmPasswordEdit->text();

        if (username.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Username cannot be empty"));
            return;
        }

        if (!password.isEmpty() && password != confirmPassword) {
            QMessageBox::warning(this, tr("Error"), tr("Passwords do not match"));
            return;
        }

        // Check if username changed and if new username already exists
        if (username != currentUsername && m_dbHandler->usernameExists(username)) {
            QMessageBox::warning(this, tr("Error"), tr("Username already exists"));
            return;
        }

        // Hash the password with MD5 if a new password was entered
        QString hashedPassword = password;
        if (!password.isEmpty()) {
            QCryptographicHash md5(QCryptographicHash::Md5);
            md5.addData(password.toUtf8());
            hashedPassword = md5.result().toHex();
        }

        bool success = m_dbHandler->updateUser(userId, username, name, hashedPassword, enabledCheck->isChecked(), creatorCheck->isChecked());

        if (success) {
            appendLog(tr("User '%1' updated successfully").arg(username), "INFO");
            refreshUsersList();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to update user"));
            appendLog(tr("Failed to update user '%1'").arg(username), "ERROR");
        }
    }
}

void TupServerWindow::removeUser()
{
    int currentRow = m_registeredUsersTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, tr("Information"), tr("Please select a user to remove"));
        return;
    }

    int userId = m_registeredUsersTable->item(currentRow, 0)->text().toInt();
    QString username = m_registeredUsersTable->item(currentRow, 1)->text();

    // Prevent removing admin user
    if (username == "admin") {
        QMessageBox::warning(this, tr("Warning"), tr("Cannot remove the admin user"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Confirm Removal"),
        tr("Are you sure you want to remove user '%1'?\nThis action cannot be undone.").arg(username),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool success = m_dbHandler->removeUser(userId);

        if (success) {
            appendLog(tr("User '%1' removed successfully").arg(username), "INFO");
            refreshUsersList();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to remove user"));
            appendLog(tr("Failed to remove user '%1'").arg(username), "ERROR");
        }
    }
}
