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
#include "tservertheme.h"
#include "logger.h"
#include "tapplicationproperties.h"
#include "filemanager.h"
#include "talgorithm.h"
#include "projectrenderer.h"
#include "firstlaunchwizard.h"

#include <QDesktopServices>
#include <QUrl>
#include <QProgressDialog>

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
#include <QScreen>
#include <QGuiApplication>
#include <QStyle>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QNetworkInterface>
#include <QCryptographicHash>
#include <QListWidget>
#include <QDomDocument>

#include <QDialog>
#include <QDateEdit>
// #include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QDate>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QStringList>
#include <QList>

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
    m_server(nullptr), m_serverRunning(false), m_dbHandler(nullptr),
    m_renderProjectButton(nullptr), m_watchProjectButton(nullptr),

    m_gradeProjectButton(nullptr), m_projectRenderer(nullptr)
{
    setWindowTitle(tr("TupiTube Server"));
    setWindowIcon(QIcon(":/icons/tupitube_server.png"));
    setMinimumSize(700, 500);

    m_server = new TcpServer(this);
    m_dbHandler = new DatabaseHandler();
    m_projectRenderer = new ProjectRenderer(m_dbHandler, this);

    // Connect server signals
    connect(m_server, &TcpServer::connectionCountChanged, this, &TupServerWindow::onConnectionCountChanged);
    connect(m_server, &TcpServer::studentConnected, this, &TupServerWindow::onStudentConnected);
    connect(m_server, &TcpServer::studentDisconnected, this, &TupServerWindow::onStudentDisconnected);
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

    // Ensure DB schema exists before launching the wizard
    if (m_dbHandler) m_dbHandler->createDatabaseSchema();

    // --- First launch detection ---
    bool firstLaunch = TCONFIG->value("FirstLaunch", true).toBool();
    if (firstLaunch) {
        FirstLaunchWizard wizard(this);
        if (wizard.exec() == QDialog::Accepted) {
            // Retrieve data from wizard fields
            QString className = wizard.field("className").toString();
            QString periodName = wizard.field("periodName").toString();
            QString studentName = wizard.field("studentName").toString();

            // 1. Create Class (use current year, empty description)
            int year = QDate::currentDate().year();
            bool classOk = m_dbHandler->addClass(className, year, "");
            // ...existing code...
        }
    }
}

// Center the main window after it is shown
void TupServerWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
#else
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
#endif
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
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

    QAction *studentsTabAction = viewMenu->addAction(tr("&Students"));
    studentsTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
    connect(studentsTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(1);
    });

    QAction *projectsTabAction = viewMenu->addAction(tr("&Projects"));
    projectsTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
    connect(projectsTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(2);
    });

    QAction *logsTabAction = viewMenu->addAction(tr("&Logs"));
    logsTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));
    connect(logsTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(3);
    });

    QAction *settingsTabAction = viewMenu->addAction(tr("Se&ttings"));
    settingsTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_5));
    connect(settingsTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(4);
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
    m_studentsTab = new QWidget();
    m_projectsTab = new QWidget();
    m_logsTab = new QWidget();
    m_classesTab = new QWidget();
    m_settingsTab = new QWidget();

    setupStatusTab();
    setupStudentsTab();
    setupProjectsTab();
    setupLogsTab();
    setupClassesTab();
    setupSettingsTab();

    m_tabWidget->addTab(m_statusTab, QIcon(":/icons/status.png"), tr("Status"));
    m_tabWidget->addTab(m_logsTab, QIcon(":/icons/logs.png"), tr("Logs"));
    m_tabWidget->addTab(m_classesTab, QIcon(":/icons/class.png"), tr("Classes"));
    m_tabWidget->addTab(m_studentsTab, QIcon(":/icons/students.png"), tr("Students"));
    m_tabWidget->addTab(m_projectsTab, QIcon(":/icons/project.png"), tr("Projects"));
    m_tabWidget->addTab(m_settingsTab, QIcon(":/icons/settings.png"), tr("Settings"));
}

void TupServerWindow::setupClassesTab()
{
    QVBoxLayout *layout = new QVBoxLayout(m_classesTab);

    // Classes Section
    QGroupBox *classesGroup = new QGroupBox(tr("Classes"));
    QVBoxLayout *classesLayout = new QVBoxLayout(classesGroup);

    m_classesTable = new QTableWidget(0, 3);
    m_classesTable->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Year")});
    m_classesTable->setColumnWidth(1, 220); // Name column wider
    m_classesTable->setColumnWidth(2, 80);  // Year column narrower
    m_classesTable->horizontalHeader()->setStretchLastSection(true);
    m_classesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_classesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_classesTable->setAlternatingRowColors(true);
    classesLayout->addWidget(m_classesTable);

    QHBoxLayout *classBtnLayout = new QHBoxLayout();
    m_addClassButton = new QPushButton(tr("Add Class"));
    m_editClassButton = new QPushButton(tr("Edit Class"));
    m_removeClassButton = new QPushButton(tr("Remove Class"));
    // m_refreshClassesButton removed
    classBtnLayout->addWidget(m_addClassButton);
    classBtnLayout->addWidget(m_editClassButton);
    classBtnLayout->addWidget(m_removeClassButton);
    classBtnLayout->addStretch();
    classesLayout->addLayout(classBtnLayout);

    layout->addWidget(classesGroup);

    // Periods Section
    QGroupBox *periodsGroup = new QGroupBox(tr("Periods"));
    QVBoxLayout *periodsLayout = new QVBoxLayout(periodsGroup);

    m_periodsTable = new QTableWidget(0, 4);
    m_periodsTable->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Year"), tr("Dates")});
    m_periodsTable->setColumnWidth(1, 220); // Name column wider
    m_periodsTable->setColumnWidth(3, 120); // Dates column narrower
    m_periodsTable->horizontalHeader()->setStretchLastSection(true);
    m_periodsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_periodsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_periodsTable->setAlternatingRowColors(true);
    periodsLayout->addWidget(m_periodsTable);

    QHBoxLayout *periodBtnLayout = new QHBoxLayout();
    m_addPeriodButton = new QPushButton(tr("Add Period"));
    m_editPeriodButton = new QPushButton(tr("Edit Period"));
    m_removePeriodButton = new QPushButton(tr("Remove Period"));
    periodBtnLayout->addWidget(m_addPeriodButton);
    periodBtnLayout->addWidget(m_editPeriodButton);
    periodBtnLayout->addWidget(m_removePeriodButton);
    periodBtnLayout->addStretch();
    periodsLayout->addLayout(periodBtnLayout);

    layout->addWidget(periodsGroup);
    layout->addStretch();

    // Connect buttons to slots
    connect(m_addClassButton, &QPushButton::clicked, this, &TupServerWindow::onAddClass);
    connect(m_editClassButton, &QPushButton::clicked, this, &TupServerWindow::onEditClass);
    connect(m_removeClassButton, &QPushButton::clicked, this, &TupServerWindow::onRemoveClass);
    connect(m_classesTable, &QTableWidget::doubleClicked, this, &TupServerWindow::onEditClass);
    connect(m_addPeriodButton, &QPushButton::clicked, this, &TupServerWindow::onAddPeriod);
    connect(m_editPeriodButton, &QPushButton::clicked, this, &TupServerWindow::onEditPeriod);
    connect(m_periodsTable, &QTableWidget::doubleClicked, this, &TupServerWindow::onEditPeriod);
    connect(m_removePeriodButton, &QPushButton::clicked, this, &TupServerWindow::onRemovePeriod);

    // Initial load
    refreshClassesList();
    refreshPeriodsList();
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
    QHBoxLayout *infoGroupLayout = new QHBoxLayout(infoGroup);

    // Form layout for status fields on the left
    QFormLayout *infoLayout = new QFormLayout();
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

    infoGroupLayout->addLayout(infoLayout);
    infoGroupLayout->addStretch();

    // Broadcast button at right side inside the group box
    m_broadcastButton = new QPushButton();
    m_broadcastButton->setIcon(QIcon(":/icons/broadcast.png"));
    m_broadcastButton->setIconSize(QSize(40, 40));
    m_broadcastButton->setFixedSize(60, 60);
    m_broadcastButton->setToolTip(tr("Broadcast Message"));
    m_broadcastButton->setEnabled(false);
    m_broadcastButton->setStyleSheet(
        "QPushButton { border-radius: 30px; background-color: #27ae60; border: none; outline: none; padding: 0px; }"
        "QPushButton:hover { background-color: #219a52; }"
        "QPushButton:pressed { background-color: #1e8449; }"
        "QPushButton:disabled { background-color: #ffffff; }"
        "QPushButton:focus { outline: none; }"
    );
    connect(m_broadcastButton, &QPushButton::clicked, this, &TupServerWindow::sendBroadcastMessage);
    infoGroupLayout->addWidget(m_broadcastButton, 0, Qt::AlignVCenter);

    layout->addWidget(infoGroup);
    layout->addStretch();
}

void TupServerWindow::setupStudentsTab()
{
    QVBoxLayout *layout = new QVBoxLayout(m_studentsTab);

    // Connected Students Section
    QGroupBox *connectedGroup = new QGroupBox(tr("Connected Students"));
    QVBoxLayout *connectedLayout = new QVBoxLayout(connectedGroup);

    m_connectedStudentsTable = new QTableWidget(0, 4);
    m_connectedStudentsTable->setHorizontalHeaderLabels({tr("Student Name"), tr("IP Address"), tr("Connected At"), tr("Status")});
    m_connectedStudentsTable->horizontalHeader()->setStretchLastSection(true);
    m_connectedStudentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_connectedStudentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_connectedStudentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_connectedStudentsTable->setAlternatingRowColors(true);
    m_connectedStudentsTable->setMaximumHeight(150);

    connectedLayout->addWidget(m_connectedStudentsTable);
    layout->addWidget(connectedGroup);

    // Registered Students Section
    QGroupBox *registeredGroup = new QGroupBox(tr("Registered Students"));
    QVBoxLayout *registeredLayout = new QVBoxLayout(registeredGroup);

    // Filter line edit
    m_studentFilterEdit = new QLineEdit();
    m_studentFilterEdit->setPlaceholderText(tr("Filter students by student name, name, or class..."));
    registeredLayout->addWidget(m_studentFilterEdit);
    connect(m_studentFilterEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        refreshStudentsList(text);
    });

    m_registeredStudentsTable = new QTableWidget(0, 6);
    m_registeredStudentsTable->setHorizontalHeaderLabels({tr("ID"), tr("Username"), tr("Full Name"), tr("Class"), tr("Enabled"), tr("Creator")});
    m_registeredStudentsTable->horizontalHeader()->setStretchLastSection(true);
    m_registeredStudentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_registeredStudentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_registeredStudentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_registeredStudentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_registeredStudentsTable->setAlternatingRowColors(true);
    m_registeredStudentsTable->setColumnHidden(0, true); // Hide ID column

    registeredLayout->addWidget(m_registeredStudentsTable);

    // Enable double-click to edit student
    connect(m_registeredStudentsTable, &QTableWidget::itemDoubleClicked, this, &TupServerWindow::editStudent);

    // Buttons for student management
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_addStudentButton = new QPushButton(tr("Add Student"));
    m_addStudentButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    connect(m_addStudentButton, &QPushButton::clicked, this, &TupServerWindow::addStudent);
    buttonLayout->addWidget(m_addStudentButton);

    m_editStudentButton = new QPushButton(tr("Edit Student"));
    m_editStudentButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(m_editStudentButton, &QPushButton::clicked, this, &TupServerWindow::editStudent);
    buttonLayout->addWidget(m_editStudentButton);

    m_removeStudentButton = new QPushButton(tr("Remove Student"));
    m_removeStudentButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    connect(m_removeStudentButton, &QPushButton::clicked, this, &TupServerWindow::removeStudent);
    buttonLayout->addWidget(m_removeStudentButton);

    m_importCsvButton = new QPushButton(tr("Import Students"));
    m_importCsvButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(m_importCsvButton, &QPushButton::clicked, this, &TupServerWindow::importUsersFromCsv);
    buttonLayout->addWidget(m_importCsvButton);
    buttonLayout->addStretch();

    // m_refreshStudentsButton removed
    // buttonLayout->addWidget(m_refreshStudentsButton);

    registeredLayout->addLayout(buttonLayout);
    layout->addWidget(registeredGroup);

    // Note: Student list will be loaded when server starts (database must be open first)
}

void TupServerWindow::setupProjectsTab()
{
    QVBoxLayout *layout = new QVBoxLayout(m_projectsTab);

    // Projects Section
    QGroupBox *projectsGroup = new QGroupBox(tr("Projects"));
    QVBoxLayout *projectsLayout = new QVBoxLayout(projectsGroup);

    // Filter line edit for Projects
    m_projectFilterEdit = new QLineEdit();
    m_projectFilterEdit->setPlaceholderText(tr("Filter projects by title, owner, or shared status..."));
    projectsLayout->addWidget(m_projectFilterEdit);
    connect(m_projectFilterEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        refreshProjectsList(text);
    });

    m_projectsTable = new QTableWidget(0, 7);
    m_projectsTable->setHorizontalHeaderLabels({tr("ID"), tr("Title"), tr("Owner"), tr("Shared"), tr("Rendered"), tr("Grade"), tr("Created")});
    m_projectsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_projectsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // Title
    m_projectsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed); // Shared
    m_projectsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed); // Rendered
    m_projectsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed); // Grade
    m_projectsTable->setColumnWidth(1, 220); // Title column wider
    m_projectsTable->setColumnWidth(3, 60);  // Shared column
    m_projectsTable->setColumnWidth(4, 80);  // Rendered column
    m_projectsTable->setColumnWidth(5, 60);  // Grade column
    m_projectsTable->horizontalHeader()->setStretchLastSection(true);
    m_projectsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_projectsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_projectsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_projectsTable->setAlternatingRowColors(true);
    m_projectsTable->setColumnHidden(0, true); // Hide ID column
    connect(m_projectsTable, &QTableWidget::itemSelectionChanged, this, &TupServerWindow::onProjectSelectionChanged);
    connect(m_projectsTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem*) {
        manageCollaborators();
    });

    projectsLayout->addWidget(m_projectsTable);

    // Project buttons
    QHBoxLayout *projectButtonLayout = new QHBoxLayout();

    m_createProjectButton = new QPushButton(tr("Create Project"));
    m_createProjectButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    connect(m_createProjectButton, &QPushButton::clicked, this, &TupServerWindow::createProject);
    projectButtonLayout->addWidget(m_createProjectButton);

    m_manageCollaboratorsButton = new QPushButton(tr("Manage Collaborators"));
    m_manageCollaboratorsButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_manageCollaboratorsButton->setEnabled(false);
    connect(m_manageCollaboratorsButton, &QPushButton::clicked, this, &TupServerWindow::manageCollaborators);
    projectButtonLayout->addWidget(m_manageCollaboratorsButton);

    m_viewChatButton = new QPushButton(tr("View Chat"));
    m_viewChatButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    m_viewChatButton->setEnabled(false);
    connect(m_viewChatButton, &QPushButton::clicked, this, &TupServerWindow::viewProjectChat);
    projectButtonLayout->addWidget(m_viewChatButton);

    m_removeProjectButton = new QPushButton(tr("Remove Project"));
    m_removeProjectButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_removeProjectButton->setEnabled(false);
    connect(m_removeProjectButton, &QPushButton::clicked, this, &TupServerWindow::removeProject);
    projectButtonLayout->addWidget(m_removeProjectButton);

    m_renderProjectButton = new QPushButton(tr("Render"));
    m_renderProjectButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_renderProjectButton->setEnabled(false);
    m_renderProjectButton->setToolTip(tr("Render selected project to MP4"));
    connect(m_renderProjectButton, &QPushButton::clicked, this, &TupServerWindow::renderProject);
    projectButtonLayout->addWidget(m_renderProjectButton);

    m_watchProjectButton = new QPushButton(tr("Watch"));
    m_watchProjectButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    m_watchProjectButton->setEnabled(false);
    m_watchProjectButton->setToolTip(tr("Open rendered MP4 in default video player"));
    connect(m_watchProjectButton, &QPushButton::clicked, this, &TupServerWindow::watchProject);
    projectButtonLayout->addWidget(m_watchProjectButton);

    m_gradeProjectButton = new QPushButton(tr("Grade"));
    m_gradeProjectButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_gradeProjectButton->setEnabled(false);
    m_gradeProjectButton->setToolTip(tr("Assign a grade to this project"));
    connect(m_gradeProjectButton, &QPushButton::clicked, this, &TupServerWindow::gradeProject);
    projectButtonLayout->addWidget(m_gradeProjectButton);

    projectButtonLayout->addStretch();


    projectsLayout->addLayout(projectButtonLayout);
    layout->addWidget(projectsGroup);

    // Collaborators Section
    QGroupBox *collaboratorsGroup = new QGroupBox(tr("Project Collaborators"));
    QVBoxLayout *collaboratorsLayout = new QVBoxLayout(collaboratorsGroup);

    m_collaboratorsTable = new QTableWidget(0, 3);
    m_collaboratorsTable->setHorizontalHeaderLabels({tr("Student Name"), tr("Full Name"), tr("Permission")});
    m_collaboratorsTable->horizontalHeader()->setStretchLastSection(true);
    m_collaboratorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_collaboratorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_collaboratorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_collaboratorsTable->setAlternatingRowColors(true);
    m_collaboratorsTable->setMaximumHeight(150);

    collaboratorsLayout->addWidget(m_collaboratorsTable);
    layout->addWidget(collaboratorsGroup);

    // Note: Project list will be loaded when server starts
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


    m_projectsPathLabel = new QLabel();
    m_projectsPathLabel->setStyleSheet("color: gray;");
    storageLayout->addRow(tr("Projects:"), m_projectsPathLabel);

    m_cachePathLabel = new QLabel();
    m_cachePathLabel->setStyleSheet("color: gray;");
    storageLayout->addRow(tr("Cache:"), m_cachePathLabel);

    m_renderPathLabel = new QLabel();
    m_renderPathLabel->setStyleSheet("color: gray;");
    storageLayout->addRow(tr("Render:"), m_renderPathLabel);

    layout->addWidget(storageGroup);

    // Language settings
    QGroupBox *languageGroup = new QGroupBox(tr("Language Settings"));
    QFormLayout *languageLayout = new QFormLayout(languageGroup);

    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem("Español", "es");
    languageLayout->addRow(tr("Language:"), m_languageCombo);

    layout->addWidget(languageGroup);

    // Theme settings
    QGroupBox *themeGroup = new QGroupBox(tr("Theme Settings"));
    QFormLayout *themeLayout = new QFormLayout(themeGroup);

    m_themeCombo = new QComboBox();
    m_themeCombo->addItem(tr("Dark"), DARK_THEME);
    m_themeCombo->addItem(tr("Light"), LIGHT_THEME);
    themeLayout->addRow(tr("UI Theme:"), m_themeCombo);

    QLabel *themeNote = new QLabel(tr("Theme changes will take effect after restart."));
    themeNote->setStyleSheet("color: gray; font-style: italic;");
    themeLayout->addRow("", themeNote);

    layout->addWidget(themeGroup);

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

    // Load Theme
    TCONFIG->beginGroup("Theme");
    int uiTheme = TCONFIG->value("UITheme", DARK_THEME).toInt();
    int themeIndex = m_themeCombo->findData(uiTheme);
    if (themeIndex >= 0)
        m_themeCombo->setCurrentIndex(themeIndex);
    TCONFIG->endGroup();

    // Update derived path labels
    updateDerivedPaths();

    // Update display labels
    QString displayHost = m_hostCombo->currentText();
    if (displayHost.contains(" ("))
        displayHost = displayHost.section(" (", 0, 0);
    m_hostLabel->setText(displayHost);
    m_portLabel->setText(QString::number(m_portSpin->value()));

    // m_server->setHost(...) and setPort(...) removed: TcpServer does not have these methods.
    // If you need to set host/port, use m_server->openConnection(m_hostCombo->currentText(), m_portSpin->value()) or store values for later use.
}

void TupServerWindow::updateDerivedPaths()
{
    QString dataPath = m_dataPathEdit->text();
    m_databasePathLabel->setText(dataPath + "/sqlite");
    m_cachePathLabel->setText(dataPath + "/cache");
    m_projectsPathLabel->setText(dataPath + "/students");
    m_renderPathLabel->setText(dataPath + "/render");
}

void TupServerWindow::saveSettings()
{
    if (m_serverRunning) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Restart Required"));
        msgBox.setText(tr("To activate these changes the app must be restarted. Are you sure?"));
        QPushButton *applyButton = msgBox.addButton(tr("Apply"), QMessageBox::AcceptRole);
        QPushButton *cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
        msgBox.setDefaultButton(cancelButton);
        msgBox.exec();
        if (msgBox.clickedButton() == applyButton) {
            saveConfigSettings();
            qApp->quit();
        }   
        
        return;        
    }

    // Save all config settings
    saveConfigSettings();

    // Update display labels
    QString hostText = m_hostCombo->currentText();
    if (hostText.contains(" ("))
        hostText = hostText.section(" (", 0, 0);
    m_hostLabel->setText(hostText);
    m_portLabel->setText(QString::number(m_portSpin->value()));

    QString newLanguage = m_languageCombo->currentData().toString();
    int newTheme = m_themeCombo->currentData().toInt();
    QString oldLanguage = TCONFIG->value("Language", "en").toString();
    int oldTheme = TCONFIG->value("UITheme", DARK_THEME).toInt();

    QString message = tr("Settings have been saved.");
    if (newLanguage != oldLanguage || newTheme != oldTheme) {
        message += "\n\n" + tr("Some changes will take effect after restarting the application.");
    } else {
        message += " " + tr("They will take effect when the server is restarted.");
    }

    appendLog(tr("Settings saved successfully"), "INFO");
    QMessageBox::information(this, tr("Settings Saved"), message);
}

void TupServerWindow::saveConfigSettings()
{
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
    TCONFIG->setValue("Language", newLanguage);
    TCONFIG->endGroup();

    TCONFIG->beginGroup("Theme");
    int newTheme = m_themeCombo->currentData().toInt();
    TCONFIG->setValue("UITheme", newTheme);
    TCONFIG->setValue("BgColor", TServerTheme::defaultBgColor(newTheme));
    TCONFIG->endGroup();


    TCONFIG->beginGroup("Database");
    TCONFIG->setValue("DatabasePath", dataPath + "/sqlite");
    TCONFIG->endGroup();

    TCONFIG->beginGroup("Cache");
    TCONFIG->setValue("CachePath", dataPath + "/cache");
    TCONFIG->endGroup();

    TCONFIG->beginGroup("Projects");
    TCONFIG->setValue("ProjectsPath", dataPath + "/students");
    TCONFIG->endGroup();

    // Add Render path to config
    TCONFIG->beginGroup("Render");
    TCONFIG->setValue("RenderPath", dataPath + "/render");
    TCONFIG->endGroup();

    TCONFIG->sync();
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

    // Create students directory if needed
    QString studentsPath = dataPath + "/students";
    QDir studentsDir(studentsPath);
    if (!studentsDir.exists()) {
        if (!studentsDir.mkpath(studentsPath)) {
            appendLog(tr("Failed to create students directory: %1").arg(studentsPath), "ERROR");
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to create students directory: %1").arg(studentsPath));
            return;
        }
    }

    // Create render directory if needed
    QString renderPath = dataPath + "/render";
    QDir renderDir(renderPath);
    if (!renderDir.exists()) {
        if (!renderDir.mkpath(renderPath)) {
            appendLog(tr("Failed to create render directory: %1").arg(renderPath), "ERROR");
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to create render directory: %1").arg(renderPath));
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
    m_statusLabel->setStyleSheet("QLabel { background-color: #27ae60; color: white; font-weight: bold; font-size: 14px; padding: 4px 8px; border-radius: 4px; }");

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

    // Load registered students and projects now that database is open
    refreshStudentsList();
    refreshProjectsList({});
}

void TupServerWindow::onServerStopped()
{
    m_serverRunning = false;

    m_toggleButton->setText(tr("Start Server"));
    m_toggleButton->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; }");

    m_broadcastButton->setEnabled(false);

    m_statusLabel->setText(tr("Stopped"));
    m_statusLabel->setStyleSheet("QLabel { background-color: #c0392b; color: white; font-weight: bold; font-size: 14px; padding: 4px 8px; border-radius: 4px; }");

    m_connectionCountLabel->setText("0");
    m_uptimeLabel->setText("00:00:00");

    // Clear connected students table
    m_connectedStudentsTable->setRowCount(0);

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
    m_broadcastButton->setEnabled(count > 0);
    updateTrayTooltip();
}

void TupServerWindow::onStudentConnected(const QString &studentname, const QString &ip)
{
    int row = m_connectedStudentsTable->rowCount();
    m_connectedStudentsTable->insertRow(row);

    m_connectedStudentsTable->setItem(row, 0, new QTableWidgetItem(studentname));
    m_connectedStudentsTable->setItem(row, 1, new QTableWidgetItem(ip));
    m_connectedStudentsTable->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    m_connectedStudentsTable->setItem(row, 3, new QTableWidgetItem(tr("Connected")));

    appendLog(tr("Student connected: %1 from %2").arg(studentname).arg(ip), "INFO");
}

void TupServerWindow::onStudentDisconnected(const QString &studentname)
{
    // Find and remove student from table
    for (int row = 0; row < m_connectedStudentsTable->rowCount(); ++row) {
        QTableWidgetItem *item = m_connectedStudentsTable->item(row, 0);
        if (item && item->text() == studentname) {
            m_connectedStudentsTable->removeRow(row);
            break;
        }
    }

    appendLog(tr("Student disconnected: %1").arg(studentname), "INFO");
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
    QString connections = m_serverRunning ? QString::number(m_connectedStudentsTable->rowCount()) : "0";
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

void TupServerWindow::refreshStudentsList(const QString &filter)
{
    m_registeredStudentsTable->setRowCount(0);

    if (!m_dbHandler) {
        appendLog(tr("Database handler not initialized"), "ERROR");
        return;
    }

    QList<DatabaseHandler::StudentInfo> students = m_dbHandler->getAllStudents();
    int filteredCount = 0;
    for (const DatabaseHandler::StudentInfo &student : students) {
        // Filter by studentname, name, or class
        if (!filter.isEmpty()) {
            QString f = filter.trimmed();
            if (!student.studentname.contains(f, Qt::CaseInsensitive) &&
                !student.name.contains(f, Qt::CaseInsensitive) &&
                !student.className.contains(f, Qt::CaseInsensitive)) {
                continue;
            }
        }
        int row = m_registeredStudentsTable->rowCount();
        m_registeredStudentsTable->insertRow(row);

        QList<QTableWidgetItem*> items;
        items << new QTableWidgetItem(QString::number(student.studentId));
        items << new QTableWidgetItem(student.studentname);
        items << new QTableWidgetItem(student.name);
        items << new QTableWidgetItem(student.className); // New Class column
        items << new QTableWidgetItem(student.isEnabled ? tr("Yes") : tr("No"));
        items << new QTableWidgetItem(student.isCreator ? tr("Yes") : tr("No"));

        if (!student.isEnabled) {
            QColor fg(200, 0, 0);     // red text for disabled
            for (QTableWidgetItem* item : items) {
                item->setData(Qt::ForegroundRole, fg);
            }
        }
        for (int col = 0; col < items.size(); ++col) {
            m_registeredStudentsTable->setItem(row, col, items[col]);
        }
        ++filteredCount;
    }

    appendLog(tr("Student list refreshed: %1 students found").arg(filteredCount), "INFO");
}

void TupServerWindow::addStudent()
{
    // Check for at least one Class and one Period before allowing user creation
    QList<DatabaseHandler::ClassInfo> classes = m_dbHandler->getAllClasses();
    QList<DatabaseHandler::PeriodInfo> periods = m_dbHandler->getAllPeriods();
    if (classes.isEmpty() || periods.isEmpty()) {
        QString missing;
        bool needClass = classes.isEmpty();
        bool needPeriod = periods.isEmpty();
        if (needClass && needPeriod) {
            missing = tr("No Classes and Periods found. Please create at least one Class and one Period before adding a student.");
        } else if (needClass) {
            missing = tr("No Classes found. Please create at least one Class before adding a student.");
        } else {
            missing = tr("No Periods found. Please create at least one Period before adding a student.");
        }
        QMessageBox::warning(this, tr("Cannot Add Student"), missing);
        // After Ok, activate Classes tab and open the appropriate dialog
        if (m_tabWidget && m_classesTab) {
            int idx = m_tabWidget->indexOf(m_classesTab);
            if (idx != -1) {
                m_tabWidget->setCurrentIndex(idx);
                // Open Add Class or Add Period dialog as needed
                if (needClass) {
                    onAddClass();
                } else if (needPeriod) {
                    onAddPeriod();
                }
            }
        }
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add New Student"));
    dialog.setMinimumWidth(350);

    QFormLayout *formLayout = new QFormLayout(&dialog);

    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(tr("Enter full name"));
    formLayout->addRow(tr("Full Name:"), nameEdit);

    QComboBox *classCombo = new QComboBox();
    classCombo->setEditable(false);
    for (const auto &c : classes) {
        classCombo->addItem(QString("%1 (%2)").arg(c.name).arg(c.year), c.classId);
    }
    formLayout->addRow(tr("Class:"), classCombo);

    QLineEdit *studentnameEdit = new QLineEdit();
    studentnameEdit->setPlaceholderText(tr("Enter username"));
    formLayout->addRow(tr("Username:"), studentnameEdit);

    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Enter password"));
    formLayout->addRow(tr("Password:"), passwordEdit);

    QLineEdit *confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText(tr("Confirm password"));
    formLayout->addRow(tr("Confirm Password:"), confirmPasswordEdit);

    QLabel *passwordMatchLabel = new QLabel();
    passwordMatchLabel->setStyleSheet("color: red;");
    formLayout->addRow("", passwordMatchLabel);

    QCheckBox *enabledCheck = new QCheckBox(tr("Account enabled"));
    enabledCheck->setChecked(true);
    formLayout->addRow("", enabledCheck);

    QCheckBox *creatorCheck = new QCheckBox(tr("Can create projects"));
    creatorCheck->setChecked(true);
    formLayout->addRow("", creatorCheck);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    formLayout->addRow(buttonBox);

    auto validatePasswords = [passwordEdit, confirmPasswordEdit, passwordMatchLabel, okButton, studentnameEdit]() {
        QString pwd = passwordEdit->text();
        QString confirm = confirmPasswordEdit->text();
        bool studentnameValid = !studentnameEdit->text().trimmed().isEmpty();
        bool passwordValid = !pwd.isEmpty();
        bool match = (pwd == confirm);

        if (pwd.isEmpty() && confirm.isEmpty()) {
            passwordMatchLabel->setText("");
        } else if (!passwordValid) {
            passwordMatchLabel->setText(QObject::tr("Password required"));
            passwordMatchLabel->setStyleSheet("color: red;");
        } else if (!match) {
            passwordMatchLabel->setText(QObject::tr("Passwords do not match"));
            passwordMatchLabel->setStyleSheet("color: red;");
        } else {
            passwordMatchLabel->setText(QObject::tr("Passwords match"));
            passwordMatchLabel->setStyleSheet("color: green;");
        }
        okButton->setEnabled(studentnameValid && passwordValid && match);
    };

    connect(passwordEdit, &QLineEdit::textChanged, validatePasswords);
    connect(confirmPasswordEdit, &QLineEdit::textChanged, validatePasswords);
    connect(studentnameEdit, &QLineEdit::textChanged, validatePasswords);

    if (dialog.exec() == QDialog::Accepted) {
        QString studentname = studentnameEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        if (classCombo->currentIndex() < 0) {
            QMessageBox::warning(this, tr("Input Error"), tr("You must select a class."));
            return;
        }
        QString className = classCombo->currentText().section(" (", 0, 0);
        QString password = passwordEdit->text();
        QString confirmPassword = confirmPasswordEdit->text();

        if (m_dbHandler->studentnameExists(studentname)) {
            QMessageBox::warning(this, tr("Error"), tr("Student name already exists"));
            return;
        }

        // Hash the password with MD5 to match client-side hashing
        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData(password.toUtf8());
        QString hashedPassword = md5.result().toHex();

        bool success = m_dbHandler->addStudent(studentname, name, hashedPassword, enabledCheck->isChecked(), creatorCheck->isChecked(), className);

        if (success) {
            appendLog(tr("Student '%1' added successfully").arg(studentname), "INFO");
            refreshStudentsList();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to add student"));
            appendLog(tr("Failed to add student '%1'").arg(studentname), "ERROR");
        }
    }
}

void TupServerWindow::editStudent()
{
    int currentRow = m_registeredStudentsTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, tr("Information"), tr("Please select a student to edit"));
        return;
    }

    int studentId = m_registeredStudentsTable->item(currentRow, 0)->text().toInt();
    QString currentStudentName = m_registeredStudentsTable->item(currentRow, 1)->text();
    QString currentName = m_registeredStudentsTable->item(currentRow, 2)->text();
    QString currentClass = m_registeredStudentsTable->item(currentRow, 3)->text();
    bool currentEnabled = m_registeredStudentsTable->item(currentRow, 4)->text() == tr("Yes");
    bool currentCreator = m_registeredStudentsTable->item(currentRow, 5)->text() == tr("Yes");

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Student: %1").arg(currentStudentName));
    dialog.setMinimumWidth(350);

    QFormLayout *formLayout = new QFormLayout(&dialog);

    QLineEdit *nameEdit = new QLineEdit(currentName);
    formLayout->addRow(tr("Full Name:"), nameEdit);

    QComboBox *classCombo = new QComboBox();
    classCombo->setEditable(false);
    QList<DatabaseHandler::ClassInfo> classes = m_dbHandler->getAllClasses();
    int selectedIndex = 0;
    for (int i = 0; i < classes.size(); ++i) {
        const auto &c = classes[i];
        QString label = QString("%1 (%2)").arg(c.name).arg(c.year);
        classCombo->addItem(label, c.classId);
        if (c.name == currentClass) selectedIndex = i;
    }
    classCombo->setCurrentIndex(selectedIndex);
    formLayout->addRow(tr("Class:"), classCombo);

    QLineEdit *studentnameEdit = new QLineEdit(currentStudentName);
    formLayout->addRow(tr("Studentname:"), studentnameEdit);

    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Leave empty to keep current"));
    formLayout->addRow(tr("New Password:"), passwordEdit);

    QLineEdit *confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText(tr("Confirm new password"));
    formLayout->addRow(tr("Confirm Password:"), confirmPasswordEdit);

    QLabel *passwordMatchLabel = new QLabel();
    passwordMatchLabel->setStyleSheet("color: red;");
    formLayout->addRow("", passwordMatchLabel);

    QCheckBox *enabledCheck = new QCheckBox(tr("Account enabled"));
    enabledCheck->setChecked(currentEnabled);
    formLayout->addRow("", enabledCheck);

    QCheckBox *creatorCheck = new QCheckBox(tr("Can create projects"));
    creatorCheck->setChecked(currentCreator);
    formLayout->addRow("", creatorCheck);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    formLayout->addRow(buttonBox);

    auto validateEditPasswords = [passwordEdit, confirmPasswordEdit, passwordMatchLabel, okButton, studentnameEdit]() {
        QString pwd = passwordEdit->text();
        QString confirm = confirmPasswordEdit->text();
        bool studentnameValid = !studentnameEdit->text().trimmed().isEmpty();
        bool match = (pwd == confirm);

        if (pwd.isEmpty() && confirm.isEmpty()) {
            passwordMatchLabel->setText("");
            okButton->setEnabled(studentnameValid);
        } else if (!match) {
            passwordMatchLabel->setText(QObject::tr("Passwords do not match"));
            passwordMatchLabel->setStyleSheet("color: red;");
            okButton->setEnabled(false);
        } else {
            passwordMatchLabel->setText(QObject::tr("Passwords match"));
            passwordMatchLabel->setStyleSheet("color: green;");
            okButton->setEnabled(studentnameValid);
        }
    };

    connect(passwordEdit, &QLineEdit::textChanged, validateEditPasswords);
    connect(confirmPasswordEdit, &QLineEdit::textChanged, validateEditPasswords);
    connect(studentnameEdit, &QLineEdit::textChanged, validateEditPasswords);

    if (dialog.exec() == QDialog::Accepted) {
        QString studentname = studentnameEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        if (classCombo->currentIndex() < 0) {
            QMessageBox::warning(this, tr("Input Error"), tr("You must select a class."));
            return;
        }
        QString className = classCombo->currentText().section(" (", 0, 0);
        QString password = passwordEdit->text();
        QString confirmPassword = confirmPasswordEdit->text();

        // Check if studentname changed and if new studentname already exists
        if (studentname != currentStudentName && m_dbHandler->studentnameExists(studentname)) {
            QMessageBox::warning(this, tr("Error"), tr("Student name already exists"));
            return;
        }

        // Hash the password with MD5 if a new password was entered
        QString hashedPassword = password;
        if (!password.isEmpty()) {
            QCryptographicHash md5(QCryptographicHash::Md5);
            md5.addData(password.toUtf8());
            hashedPassword = md5.result().toHex();
        }

        bool success = m_dbHandler->updateStudent(studentId, studentname, name, hashedPassword, enabledCheck->isChecked(), creatorCheck->isChecked(), className);

        if (success) {
            appendLog(tr("Student '%1' updated successfully").arg(studentname), "INFO");
            refreshStudentsList();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to update student"));
            appendLog(tr("Failed to update student '%1'").arg(studentname), "ERROR");
        }
    }
}

void TupServerWindow::removeStudent()
{
    int currentRow = m_registeredStudentsTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, tr("Information"), tr("Please select a student to remove"));
        return;
    }

    int studentId = m_registeredStudentsTable->item(currentRow, 0)->text().toInt();
    QString studentname = m_registeredStudentsTable->item(currentRow, 1)->text();

    // Prevent removing admin student
    if (studentname == "admin") {
        QMessageBox::warning(this, tr("Warning"), tr("Cannot remove the admin student"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Confirm Removal"),
        tr("Are you sure you want to remove student '%1'?\nThis action cannot be undone.").arg(studentname),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool success = m_dbHandler->removeStudent(studentId);

        if (success) {
            appendLog(tr("Student '%1' removed successfully").arg(studentname), "INFO");
            refreshStudentsList();
        } else {
            // Check if the failure is due to foreign key constraint (student owns projects)
            QMessageBox::critical(this, tr("Error"),
                tr("Cannot remove student '%1' because they own one or more projects. Please reassign or delete their projects first.").arg(studentname));
            appendLog(tr("Failed to remove student '%1' (owns projects)").arg(studentname), "ERROR");
        }
    }
}

// Project collaboration management slots


void TupServerWindow::onProjectSelectionChanged()
{
    int currentRow = m_projectsTable->currentRow();
    bool hasSelection = currentRow >= 0;
    m_manageCollaboratorsButton->setEnabled(hasSelection);
    m_viewChatButton->setEnabled(hasSelection);
    m_removeProjectButton->setEnabled(hasSelection);
    m_renderProjectButton->setEnabled(hasSelection);
    m_gradeProjectButton->setEnabled(hasSelection);

    if (hasSelection) {
        // Enable Watch only if already rendered (column 4 != "No")
        QString renderedText = m_projectsTable->item(currentRow, 4)->text();
        m_watchProjectButton->setEnabled(renderedText != tr("No"));

        int projectId = m_projectsTable->item(currentRow, 0)->text().toInt();
        updateCollaboratorsDisplay(projectId);
    } else {
        m_watchProjectButton->setEnabled(false);
        m_collaboratorsTable->setRowCount(0);
    }
}

void TupServerWindow::updateCollaboratorsDisplay(int projectId)
{
    m_collaboratorsTable->setRowCount(0);

    QList<DatabaseHandler::CollaboratorInfo> collaborators = m_dbHandler->getProjectCollaborators(projectId);

    for (const DatabaseHandler::CollaboratorInfo &collab : collaborators) {
        int row = m_collaboratorsTable->rowCount();
        m_collaboratorsTable->insertRow(row);

        m_collaboratorsTable->setItem(row, 0, new QTableWidgetItem(collab.studentname));
        m_collaboratorsTable->setItem(row, 1, new QTableWidgetItem(collab.name));
        QString permission = collab.permissionLevel == 1 ? tr("Editor") : tr("Viewer");
        m_collaboratorsTable->setItem(row, 2, new QTableWidgetItem(permission));
    }
}

void TupServerWindow::createProject()
{
    // Check if there are any students in the database
    QList<DatabaseHandler::StudentInfo> students = m_dbHandler->getAllStudents();
    if (students.isEmpty()) {
        QMessageBox::warning(this, tr("No Users Found"),
            tr("You must create at least one user before creating a project.\n\nPlease add a user in the Students tab first."));
        // Switch to Students tab (index 3)
        if (m_tabWidget) m_tabWidget->setCurrentIndex(3);
        // Open the Add Student dialog
        QTimer::singleShot(0, this, SLOT(addStudent()));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Create New Project"));
    dialog.setMinimumWidth(550);
    dialog.setMinimumHeight(450);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *titleEdit = new QLineEdit();
    titleEdit->setPlaceholderText(tr("Enter project title"));
    formLayout->addRow(tr("Title:"), titleEdit);

    QLineEdit *descriptionEdit = new QLineEdit();
    descriptionEdit->setPlaceholderText(tr("Enter project description"));
    formLayout->addRow(tr("Description:"), descriptionEdit);

    // Dimension selection
    QComboBox *dimensionCombo = new QComboBox();
    dimensionCombo->addItem("1920x1080 (Full HD)", QSize(1920, 1080));
    dimensionCombo->addItem("1280x720 (HD)", QSize(1280, 720));
    dimensionCombo->addItem("854x480 (SD)", QSize(854, 480));
    dimensionCombo->addItem("640x480 (VGA)", QSize(640, 480));
    dimensionCombo->setCurrentIndex(0);
    formLayout->addRow(tr("Dimension:"), dimensionCombo);

    // FPS selection
    QSpinBox *fpsSpin = new QSpinBox();
    fpsSpin->setRange(1, 60);
    fpsSpin->setValue(12);
    formLayout->addRow(tr("FPS:"), fpsSpin);

    // Owner selection
    QComboBox *ownerCombo = new QComboBox();
    for (const DatabaseHandler::StudentInfo &student : students) {
        if (student.isEnabled && student.isCreator) {
            ownerCombo->addItem(student.studentname + " (" + student.name + ")", student.studentId);
        }
    }
    formLayout->addRow(tr("Owner:"), ownerCombo);

    // Period selection (after Owner)
    QComboBox *periodCombo = new QComboBox();
    QList<DatabaseHandler::PeriodInfo> periods = m_dbHandler->getAllPeriods();
    int bestFitIndex = -1;
    QDate currentDate = QDate::currentDate();
    for (int i = 0; i < periods.size(); ++i) {
        const DatabaseHandler::PeriodInfo &period = periods[i];
        QString label = period.name + " (" + period.startDate + " - " + period.endDate + ")";
        periodCombo->addItem(label, period.periodId);
        QDate start = QDate::fromString(period.startDate, "yyyy-MM-dd");
        QDate end = QDate::fromString(period.endDate, "yyyy-MM-dd");
        if (start.isValid() && end.isValid() && currentDate >= start && currentDate <= end && bestFitIndex == -1) {
            bestFitIndex = i;
        }
    }
    if (bestFitIndex != -1) {
        periodCombo->setCurrentIndex(bestFitIndex);
    } else if (periodCombo->count() > 0) {
        periodCombo->setCurrentIndex(0);
    }
    formLayout->addRow(tr("Period:"), periodCombo);

    mainLayout->addLayout(formLayout);

    // Collaborators selection with dual-list interface
    QGroupBox *collaboratorsGroup = new QGroupBox(tr("Assign Collaborators"));
    QHBoxLayout *collaboratorsLayout = new QHBoxLayout(collaboratorsGroup);

    // Available students list (left)
    QVBoxLayout *availableLayout = new QVBoxLayout();
    QLabel *availableLabel = new QLabel(tr("Available Students:"));
    availableLayout->addWidget(availableLabel);
    QLineEdit *searchAvailableEdit = new QLineEdit();
    searchAvailableEdit->setPlaceholderText(tr("Search students..."));
    availableLayout->addWidget(searchAvailableEdit);
    QListWidget *availableList = new QListWidget();
    availableList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    availableLayout->addWidget(availableList);
    collaboratorsLayout->addLayout(availableLayout);

    // Transfer buttons (center)
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch();
    QPushButton *addCollabButton = new QPushButton(" " + tr("Add"));
    addCollabButton->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    addCollabButton->setToolTip(tr("Add selected students as collaborators"));
    buttonLayout->addWidget(addCollabButton);
    QPushButton *removeCollabButton = new QPushButton(" " + tr("Remove"));
    removeCollabButton->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    removeCollabButton->setToolTip(tr("Remove selected collaborators"));
    buttonLayout->addWidget(removeCollabButton);
    buttonLayout->addStretch();
    collaboratorsLayout->addLayout(buttonLayout);

    // Selected collaborators list (right)
    QVBoxLayout *selectedLayout = new QVBoxLayout();
    QLabel *selectedLabel = new QLabel(tr("Selected Collaborators:"));
    selectedLayout->addWidget(selectedLabel);
    QListWidget *selectedList = new QListWidget();
    selectedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    selectedLayout->addWidget(selectedList);
    collaboratorsLayout->addLayout(selectedLayout);

    // Lambda to rebuild available list based on selected owner and search filter
    auto rebuildAvailableList = [&students, availableList, selectedList, ownerCombo, searchAvailableEdit]() {
        int currentOwnerId = ownerCombo->currentData().toInt();
        QString filter = searchAvailableEdit->text().trimmed();
        // Collect already selected student IDs
        QSet<int> selectedIds;
        for (int i = 0; i < selectedList->count(); i++) {
            selectedIds.insert(selectedList->item(i)->data(Qt::UserRole).toInt());
        }
        availableList->clear();
        for (const DatabaseHandler::StudentInfo &student : students) {
            if (student.isEnabled && student.studentId != currentOwnerId && !selectedIds.contains(student.studentId)) {
                QString displayText = student.studentname + " (" + student.name + ")";
                if (filter.isEmpty() || displayText.contains(filter, Qt::CaseInsensitive)) {
                    QListWidgetItem *item = new QListWidgetItem(displayText);
                    item->setData(Qt::UserRole, student.studentId);
                    availableList->addItem(item);
                }
            }
        }
    };

    // When owner changes, also remove them from selected list if present
    connect(ownerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            [rebuildAvailableList, selectedList, ownerCombo](int) {
        int currentOwnerId = ownerCombo->currentData().toInt();
        // Remove owner from selected collaborators if present
        for (int i = selectedList->count() - 1; i >= 0; i--) {
            if (selectedList->item(i)->data(Qt::UserRole).toInt() == currentOwnerId) {
                delete selectedList->takeItem(i);
            }
        }
        rebuildAvailableList();
    });
    // Filter available list as student types
    connect(searchAvailableEdit, &QLineEdit::textChanged, rebuildAvailableList);

    // Add button: move selected from available to selected
    connect(addCollabButton, &QPushButton::clicked, [availableList, selectedList]() {
        QList<QListWidgetItem*> items = availableList->selectedItems();
        for (QListWidgetItem *item : items) {
            QListWidgetItem *newItem = new QListWidgetItem(item->text());
            newItem->setData(Qt::UserRole, item->data(Qt::UserRole));
            selectedList->addItem(newItem);
            delete availableList->takeItem(availableList->row(item));
        }
    });

    // Remove button: move selected from selected back to available
    connect(removeCollabButton, &QPushButton::clicked, [availableList, selectedList]() {
        QList<QListWidgetItem*> items = selectedList->selectedItems();
        for (QListWidgetItem *item : items) {
            QListWidgetItem *newItem = new QListWidgetItem(item->text());
            newItem->setData(Qt::UserRole, item->data(Qt::UserRole));
            availableList->addItem(newItem);
            delete selectedList->takeItem(selectedList->row(item));
        }
    });

    // Initialize available list
    rebuildAvailableList();

    mainLayout->addWidget(collaboratorsGroup);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QString title = titleEdit->text().trimmed();
        QString description = descriptionEdit->text().trimmed();
        int ownerId = ownerCombo->currentData().toInt();
        QString ownerStudentname = ownerCombo->currentText().split(" (").first();
        QSize dimension = dimensionCombo->currentData().toSize();
        int fps = fpsSpin->value();

        if (title.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Project title cannot be empty"));
            return;
        }

        // Generate filename from timestamp
        QString filename = TAlgorithm::randomString(20);

        // Get collaborators from selected list
        QList<int> collaboratorIds;
        for (int i = 0; i < selectedList->count(); i++) {
            QListWidgetItem *item = selectedList->item(i);
            int studentId = item->data(Qt::UserRole).toInt();
            collaboratorIds.append(studentId);
        }

        // Create the actual project file on disk
        bool fileCreated = FileManager::createEmptyProjectFile(title, description, ownerStudentname, 
                                                                ownerId, filename, dimension, fps);

        if (!fileCreated) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to create project files on disk"));
            appendLog(tr("Failed to create project files for '%1'").arg(title), "ERROR");
            return;
        }

        // Get selected period
        int periodId = periodCombo->currentData().toInt();

        // Add to database with periodId
        bool dbSuccess = m_dbHandler->createEmptyProject(title, description, ownerId, filename, collaboratorIds, periodId);

        if (dbSuccess) {
            appendLog(tr("Project '%1' created with %2 collaborators").arg(title).arg(collaboratorIds.count()), "INFO");
            refreshProjectsList({});
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to add project to database"));
            appendLog(tr("Failed to add project '%1' to database").arg(title), "ERROR");
        }
    }
}

void TupServerWindow::manageCollaborators()
{
    int currentRow = m_projectsTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, tr("Information"), tr("Please select a project first"));
        return;
    }

    int projectId = m_projectsTable->item(currentRow, 0)->text().toInt();
    QString projectTitle = m_projectsTable->item(currentRow, 1)->text();
    QString ownerStudentname = m_projectsTable->item(currentRow, 2)->text();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Manage Collaborators - %1").arg(projectTitle));
    dialog.setMinimumWidth(550);
    dialog.setMinimumHeight(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QLabel *infoLabel = new QLabel(tr("Owner: %1").arg(ownerStudentname));
    infoLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(infoLabel);

    // Dual-list layout
    QHBoxLayout *listsLayout = new QHBoxLayout();

    // Available students list (left)
    QVBoxLayout *availableLayout = new QVBoxLayout();
    QLabel *availableLabel = new QLabel(tr("Available Students:"));
    availableLayout->addWidget(availableLabel);
    QLineEdit *searchAvailableEdit = new QLineEdit();
    searchAvailableEdit->setPlaceholderText(tr("Search students..."));
    availableLayout->addWidget(searchAvailableEdit);
    QListWidget *availableList = new QListWidget();
    availableList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    availableLayout->addWidget(availableList);
    listsLayout->addLayout(availableLayout);

    // Transfer buttons (center)
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch();
    QPushButton *addButton = new QPushButton(" " + tr("Add"));
    addButton->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    addButton->setToolTip(tr("Add selected students as collaborators"));
    buttonLayout->addWidget(addButton);
    QPushButton *removeButton = new QPushButton(" " + tr("Remove"));
    removeButton->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    removeButton->setToolTip(tr("Remove selected collaborators"));
    buttonLayout->addWidget(removeButton);
    // Disable remove button if currentList is empty
    auto updateRemoveButtonState = [removeButton](QListWidget *currentList) {
        removeButton->setEnabled(currentList->count() > 0);
    };
    buttonLayout->addStretch();
    listsLayout->addLayout(buttonLayout);

    // Current collaborators list (right)
    QVBoxLayout *currentLayout = new QVBoxLayout();
    QLabel *currentLabel = new QLabel(tr("Current Collaborators:"));
    currentLayout->addWidget(currentLabel);
    QListWidget *currentList = new QListWidget();
    currentList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    currentLayout->addWidget(currentList);
    listsLayout->addLayout(currentLayout);
    // Initial state
    updateRemoveButtonState(currentList);
    // Update state on changes
    QObject::connect(currentList->model(), &QAbstractItemModel::rowsInserted, [currentList, updateRemoveButtonState]() { updateRemoveButtonState(currentList); });
    QObject::connect(currentList->model(), &QAbstractItemModel::rowsRemoved, [currentList, updateRemoveButtonState]() { updateRemoveButtonState(currentList); });

    mainLayout->addLayout(listsLayout);

    // Populate current collaborators
    QList<DatabaseHandler::CollaboratorInfo> collaborators = m_dbHandler->getProjectCollaborators(projectId);
    for (const DatabaseHandler::CollaboratorInfo &collab : collaborators) {
        QListWidgetItem *item = new QListWidgetItem(collab.studentname + " (" + collab.name + ")");
        item->setData(Qt::UserRole, collab.studentId);
        currentList->addItem(item);
    }

    // Populate available students
    QList<DatabaseHandler::StudentInfo> students = m_dbHandler->getAllStudents();
    QSet<int> existingCollabIds;
    for (const DatabaseHandler::CollaboratorInfo &collab : collaborators) {
        existingCollabIds.insert(collab.studentId);
    }

    // Store all available students for filtering
    QList<QPair<QString, int>> availableStudentsData;
    for (const DatabaseHandler::StudentInfo &student : students) {
        if (student.isEnabled && student.studentname != ownerStudentname && !existingCollabIds.contains(student.studentId)) {
            QString displayText = student.studentname + " (" + student.name + ")";
            availableStudentsData.append(qMakePair(displayText, student.studentId));
        }
    }
    // Helper to repopulate availableList based on filter
    auto filterAvailableList = [availableList, &availableStudentsData, searchAvailableEdit]() {
        QString filter = searchAvailableEdit->text().trimmed();
        availableList->clear();
        for (const auto &pair : availableStudentsData) {
            if (filter.isEmpty() || pair.first.contains(filter, Qt::CaseInsensitive)) {
                QListWidgetItem *item = new QListWidgetItem(pair.first);
                item->setData(Qt::UserRole, pair.second);
                availableList->addItem(item);
            }
        }
    };
    filterAvailableList();
    QObject::connect(searchAvailableEdit, &QLineEdit::textChanged, filterAvailableList);

    // Add collaborator logic
    connect(addButton, &QPushButton::clicked, [this, projectId, currentList, availableList]() {
        QList<QListWidgetItem*> selectedItems = availableList->selectedItems();
        for (QListWidgetItem *item : selectedItems) {
            int studentId = item->data(Qt::UserRole).toInt();
            if (m_dbHandler->addCollaborator(projectId, studentId)) {
                // Move to current list
                QListWidgetItem *newItem = new QListWidgetItem(item->text());
                newItem->setData(Qt::UserRole, studentId);
                currentList->addItem(newItem);
                delete availableList->takeItem(availableList->row(item));
            }
        }
        updateCollaboratorsDisplay(projectId);
        refreshProjectsList({});
    });

    // Remove collaborator logic
    connect(removeButton, &QPushButton::clicked, [this, projectId, currentList, availableList]() {
        QList<QListWidgetItem*> selectedItems = currentList->selectedItems();
        for (QListWidgetItem *item : selectedItems) {
            int studentId = item->data(Qt::UserRole).toInt();
            if (m_dbHandler->removeCollaborator(projectId, studentId)) {
                // Move to available list
                QListWidgetItem *newItem = new QListWidgetItem(item->text());
                newItem->setData(Qt::UserRole, studentId);
                availableList->addItem(newItem);
                delete currentList->takeItem(currentList->row(item));
            }
        }
        updateCollaboratorsDisplay(projectId);
        refreshProjectsList({});
    });

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    dialog.exec();
}
void TupServerWindow::removeProject()
{
    int currentRow = m_projectsTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, tr("Information"), tr("Please select a project first"));
        return;
    }

    int projectId = m_projectsTable->item(currentRow, 0)->text().toInt();
    QString projectTitle = m_projectsTable->item(currentRow, 1)->text();
    QString ownerStudentname = m_projectsTable->item(currentRow, 2)->text();

    // Confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Confirm Deletion"),
        tr("Are you sure you want to delete the project '%1' owned by '%2'?\n\n"
           "This will remove the project from the database and delete all associated files.\n"
           "This action cannot be undone.")
            .arg(projectTitle)
            .arg(ownerStudentname),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes)
        return;

    // Get project info before deletion
    QString filename = m_dbHandler->getProjectFilename(projectId);
    int ownerId = m_dbHandler->getProjectOwnerId(projectId);

    // Delete from database (also deletes collaborations)
    bool success = m_dbHandler->deleteProject(projectId);

    if (success) {
        // Delete chat history for this project
        m_dbHandler->clearChatHistory(projectId);

        // Delete project files from disk
        if (!filename.isEmpty() && ownerId > 0) {
            QString repoDir = kAppProp->repositoryDir();
            if (repoDir.endsWith("/"))
                repoDir.chop(1);
            QString projectPath = repoDir + "/" + QString::number(ownerId) + "/projects/" + filename + ".tup";
            
            QFile projectFile(projectPath);
            if (projectFile.exists()) {
                if (projectFile.remove()) {
                    appendLog(tr("Project file deleted: %1").arg(projectPath), "INFO");
                } else {
                    appendLog(tr("Warning: Could not delete project file: %1").arg(projectPath), "WARNING");
                }
            }

            // Also remove cache directory if it exists
            QString cacheDir = CACHE_DIR;
            if (cacheDir.endsWith("/"))
                cacheDir.chop(1);
            QString cachePath = cacheDir + "/" + QString::number(ownerId) + "/" + filename;
            QDir cacheDirectory(cachePath);
            if (cacheDirectory.exists()) {
                if (cacheDirectory.removeRecursively()) {
                    appendLog(tr("Project cache deleted: %1").arg(cachePath), "INFO");
                } else {
                    appendLog(tr("Warning: Could not delete project cache: %1").arg(cachePath), "WARNING");
                }
            }
        }

        appendLog(tr("Project '%1' deleted successfully").arg(projectTitle), "INFO");
        refreshProjectsList({});
        m_collaboratorsTable->setRowCount(0);
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to delete project from database"));
        appendLog(tr("Failed to delete project '%1'").arg(projectTitle), "ERROR");
    }
}

void TupServerWindow::viewProjectChat()
{
    int currentRow = m_projectsTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, tr("Information"), tr("Please select a project first"));
        return;
    }

    int projectId = m_projectsTable->item(currentRow, 0)->text().toInt();
    QString projectTitle = m_projectsTable->item(currentRow, 1)->text();

    // Load chat messages for this project
    QList<DatabaseHandler::ChatMessage> messages = m_dbHandler->getChatHistory(projectId, 1000);

    // Check if there are any messages
    if (messages.isEmpty()) {
        QMessageBox::information(this, tr("Chat History"), 
            tr("No chat messages registered for project '%1'.").arg(projectTitle));
        return;
    }

    // Create dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Chat History - %1").arg(projectTitle));
    dialog.setMinimumSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Chat messages table
    QTableWidget *chatTable = new QTableWidget();
    chatTable->setColumnCount(4);
    chatTable->setHorizontalHeaderLabels({tr("Time"), tr("Student"), tr("Type"), tr("Message")});
    chatTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    chatTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    chatTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    chatTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    chatTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    chatTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    chatTable->setAlternatingRowColors(true);
    layout->addWidget(chatTable);

    // Messages are in DESC order, reverse for display (oldest first)
    for (int i = messages.count() - 1; i >= 0; i--) {
        const DatabaseHandler::ChatMessage &msg = messages[i];
        
        int row = chatTable->rowCount();
        chatTable->insertRow(row);

        chatTable->setItem(row, 0, new QTableWidgetItem(msg.createdAt));
        chatTable->setItem(row, 1, new QTableWidgetItem(msg.studentname));
        chatTable->setItem(row, 2, new QTableWidgetItem(msg.messageType));
        chatTable->setItem(row, 3, new QTableWidgetItem(msg.message));
    }

    // Bottom buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    buttonLayout->addStretch();

    QPushButton *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    // Scroll to bottom (most recent messages)
    chatTable->scrollToBottom();

    dialog.exec();
}

void TupServerWindow::sendBroadcastMessage()
{
    if (!m_serverRunning) {
        QMessageBox::warning(this, tr("Server Not Running"),
            tr("Please start the server before broadcasting messages."));
        return;
    }

    // Check if there are connected students
    if (m_connectedStudentsTable->rowCount() == 0) {
        QMessageBox::information(this, tr("No Connected Students"),
            tr("There are no students currently connected to the server."));
        return;
    }

    // Create input dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Broadcast Message"));
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel(tr("Enter a message to send to all connected students:"));
    layout->addWidget(label);

    QTextEdit *messageEdit = new QTextEdit();
    messageEdit->setPlaceholderText(tr("Type your message here..."));
    messageEdit->setMinimumHeight(100);
    layout->addWidget(messageEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    QPushButton *sendButton = buttonBox->addButton(tr("Send"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    connect(sendButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QString message = messageEdit->toPlainText().trimmed();
        if (message.isEmpty()) {
            QMessageBox::warning(this, tr("Empty Message"),
                tr("Please enter a message to broadcast."));
            return;
        }

        // Create the communication_wall XML package
        QDomDocument doc;
        QDomElement root = doc.createElement("communication_wall");
        root.setAttribute("version", 0);
        doc.appendChild(root);

        QDomElement msgElement = doc.createElement("message");
        msgElement.setAttribute("from", tr("Teacher"));
        msgElement.setAttribute("text", message);
        root.appendChild(msgElement);

        // Broadcast to all connected clients
        m_server->sendToAll(doc);

        appendLog(tr("Broadcast message sent: %1").arg(message), "INFO");
    }
}

void TupServerWindow::importUsersFromCsv()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Students File"), QString(), tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Import Students"), tr("Failed to open file: %1").arg(fileName));
        return;
    }

    QTextStream in(&file);
    QString headerLine = in.readLine();
    if (headerLine.isNull()) {
        QMessageBox::warning(this, tr("Import Students"), tr("CSV file is empty."));
        return;
    }

    QStringList headers = headerLine.split(",");
    // Expected: Name, Class, Studentname, Password, Enabled, Creator
    int nameIdx = headers.indexOf("Name");
    int classIdx = headers.indexOf("Class");
    int studentnameIdx = headers.indexOf("Studentname");
    int passwordIdx = headers.indexOf("Password");
    int enabledIdx = headers.indexOf("Enabled");
    int creatorIdx = headers.indexOf("Creator");
    if (nameIdx == -1 || classIdx == -1 || studentnameIdx == -1 || passwordIdx == -1 || enabledIdx == -1 || creatorIdx == -1) {
        QMessageBox::warning(this, tr("Import Students"), tr("CSV header must include: Name, Class, Studentname, Password, Enabled, Creator"));
        return;
    }

    QList<QVariantMap> studentsToImport;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        QStringList fields = line.split(",");
        if (fields.size() < headers.size()) continue;
        QVariantMap student;
        student["name"] = fields[nameIdx].trimmed();
        student["class"] = fields[classIdx].trimmed();
        student["studentname"] = fields[studentnameIdx].trimmed();
        student["password"] = fields[passwordIdx].trimmed();
        student["enabled"] = fields[enabledIdx].trimmed();
        student["creator"] = fields[creatorIdx].trimmed();
        studentsToImport.append(student);
    }

    if (studentsToImport.isEmpty()) {
        QMessageBox::information(this, tr("Import Students"), tr("No valid student records found in the file."));
        return;
    }

    int insertedCount = 0;
    for (const QVariantMap &student : studentsToImport) {
        QString studentname = student["studentname"].toString();
        if (studentname.isEmpty())
            continue;
        if (m_dbHandler->studentnameExists(studentname)) {
            appendLog(tr("Skipped student '%1': studentname already exists").arg(studentname), "WARN");
            continue;
        }
        QString name = student["name"].toString();
        QString password = student["password"].toString();
        // Hash the password with MD5 to match Add/Edit Student logic
        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData(password.toUtf8());
        QString hashedPassword = md5.result().toHex();
        bool enabled = (student["enabled"].toString().toLower() == "true" || student["enabled"].toString() == "1");
        bool creator = (student["creator"].toString().toLower() == "true" || student["creator"].toString() == "1");
        QString studentClass = student["class"].toString();
        if (m_dbHandler->addStudent(studentname, name, hashedPassword, enabled, creator, studentClass)) {
            insertedCount++;
        } else {
            appendLog(tr("Failed to insert student '%1'").arg(studentname), "ERROR");
        }
    }
    appendLog(tr("CSV import complete. %1 students inserted.").arg(insertedCount), "INFO");
    QMessageBox::information(this, tr("Import Students"), tr("Import complete. %1 students inserted.").arg(insertedCount));
    refreshStudentsList(m_studentFilterEdit ? m_studentFilterEdit->text() : QString());
}

// === Classes Tab Logic ===

void TupServerWindow::refreshClassesList() {
    m_classesTable->setRowCount(0);
    QList<DatabaseHandler::ClassInfo> classes = m_dbHandler->getAllClasses();
    for (const auto &c : classes) {
        int row = m_classesTable->rowCount();
        m_classesTable->insertRow(row);
        m_classesTable->setItem(row, 0, new QTableWidgetItem(QString::number(c.classId)));
        m_classesTable->setItem(row, 1, new QTableWidgetItem(c.name));
        m_classesTable->setItem(row, 2, new QTableWidgetItem(QString::number(c.year)));
    }
}

void TupServerWindow::refreshProjectsList(const QString &filter)
{
    m_projectsTable->setRowCount(0);
    m_collaboratorsTable->setRowCount(0);

    if (!m_dbHandler) {
        appendLog(tr("Database handler not initialized"), "ERROR");
        return;
    }

    QList<DatabaseHandler::ProjectRecord> projects = m_dbHandler->getAllProjects();
    int count = 0;
    for (const DatabaseHandler::ProjectRecord &project : projects) {
        // Filter by title, owner, or shared status
        if (!filter.isEmpty()) {
            QString sharedStr = project.isShared ? tr("Yes") : tr("No");
            if (!project.title.contains(filter, Qt::CaseInsensitive) &&
                !project.ownerStudentname.contains(filter, Qt::CaseInsensitive) &&
                !sharedStr.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }
        }
        int row = m_projectsTable->rowCount();
        m_projectsTable->insertRow(row);
        m_projectsTable->setItem(row, 0, new QTableWidgetItem(QString::number(project.projectId)));
        m_projectsTable->setItem(row, 1, new QTableWidgetItem(project.title));
        m_projectsTable->setItem(row, 2, new QTableWidgetItem(project.ownerStudentname));
        m_projectsTable->setItem(row, 3, new QTableWidgetItem(project.isShared ? tr("Yes") : tr("No")));

        // Render status column
        QString renderedText = project.lastRenderedAt.isEmpty() ? tr("No") : tr("Yes");
        QTableWidgetItem *renderedItem = new QTableWidgetItem(renderedText);
        if (!project.lastRenderedAt.isEmpty())
            renderedItem->setForeground(QColor("#27ae60"));
        m_projectsTable->setItem(row, 4, renderedItem);

        // Grade column
        DatabaseHandler::GradeInfo gradeInfo = m_dbHandler->getGrade(project.projectId, project.ownerId);
        QString gradeText = gradeInfo.found ? QString::number(gradeInfo.grade, 'f', 1) : tr("-");
        m_projectsTable->setItem(row, 5, new QTableWidgetItem(gradeText));

        m_projectsTable->setItem(row, 6, new QTableWidgetItem(project.createdAt));
        ++count;
    }
    m_manageCollaboratorsButton->setEnabled(false);
    if (m_renderProjectButton) m_renderProjectButton->setEnabled(false);
    if (m_watchProjectButton)  m_watchProjectButton->setEnabled(false);
    if (m_gradeProjectButton)  m_gradeProjectButton->setEnabled(false);
    appendLog(tr("Project list refreshed: %1 projects found").arg(count), "INFO");
}

void TupServerWindow::onEditClass() {
    int row = m_classesTable->currentRow();
    if (row < 0) return;
    int classId = m_classesTable->item(row, 0)->text().toInt();
    QString name = m_classesTable->item(row, 1)->text();
    int year = m_classesTable->item(row, 2)->text().toInt();
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Class"));
    QFormLayout form(&dialog);
    QLineEdit nameEdit(name), descEdit;
    QComboBox yearCombo;
    for (int y = 2026; y <= 2032; ++y) yearCombo.addItem(QString::number(y));
    int yearIndex = yearCombo.findText(QString::number(year));
    if (yearIndex >= 0) yearCombo.setCurrentIndex(yearIndex);
    form.addRow(tr("Name:"), &nameEdit);
    form.addRow(tr("Year:"), &yearCombo);
    form.addRow(tr("Description:"), &descEdit);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        int newYear = yearCombo.currentText().toInt();
        if (m_dbHandler->updateClass(classId, nameEdit.text(), newYear, descEdit.text())) {
            refreshClassesList();
        }
    }
}

void TupServerWindow::onRemoveClass() {
    int row = m_classesTable->currentRow();
    if (row < 0) return;
    int classId = m_classesTable->item(row, 0)->text().toInt();
    // Check for students or projects linked to this class
    bool hasStudents = false, hasProjects = false;
    QList<DatabaseHandler::StudentInfo> students = m_dbHandler->getAllStudents();
    for (const auto &student : students) {
        if (student.classId == classId) {
            hasStudents = true;
            break;
        }
    }
    QList<DatabaseHandler::ProjectRecord> projects = m_dbHandler->getAllProjects();
    for (const auto &project : projects) {
        if (project.classId == classId) {
            hasProjects = true;
            break;
        }
    }
    if (hasStudents || hasProjects) {
        QString msg = tr("This class cannot be removed because it has associated ");
        if (hasStudents && hasProjects)
            msg += tr("students and projects.");
        else if (hasStudents)
            msg += tr("students.");
        else
            msg += tr("projects.");
        QMessageBox::warning(this, tr("Remove Class"), msg);
        return;
    }
    if (QMessageBox::question(this, tr("Remove Class"), tr("Are you sure?")) == QMessageBox::Yes) {
        if (m_dbHandler->removeClass(classId)) {
            refreshClassesList();
        }
    }
}

void TupServerWindow::onAddPeriod() {
    qDebug() << "[onAddPeriod] Called";
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Period"));
    qDebug() << "[onAddPeriod] Dialog created";
    dialog.setMinimumWidth(360);
    QFormLayout form(&dialog);
    QLineEdit nameEdit;
    QComboBox yearCombo;
    for (int y = 2026; y <= 2032; ++y) yearCombo.addItem(QString::number(y));
    QDateEdit startDateEdit, endDateEdit;
    startDateEdit.setCalendarPopup(true);
    endDateEdit.setCalendarPopup(true);
    startDateEdit.setDisplayFormat("yyyy-MM-dd");
    endDateEdit.setDisplayFormat("yyyy-MM-dd");
    startDateEdit.setDate(QDate::currentDate());
    endDateEdit.setDate(QDate::currentDate());
    form.addRow(tr("Name:"), &nameEdit);
    form.addRow(tr("Year:"), &yearCombo);
    form.addRow(tr("Start Date:"), &startDateEdit);
    form.addRow(tr("End Date:"), &endDateEdit);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form.addRow(&buttons);
    QPushButton *okButton = buttons.button(QDialogButtonBox::Ok);
    QLabel *dateWarning = new QLabel;
    dateWarning->setStyleSheet("color: red");
    form.addRow("", dateWarning);
    auto validateForm = [&]() {
        qDebug() << "[onAddPeriod] Validating form: name=" << nameEdit.text() << ", start=" << startDateEdit.date() << ", end=" << endDateEdit.date();
        if (nameEdit.text().trimmed().isEmpty()) {
            dateWarning->setText(tr("Name cannot be empty."));
            okButton->setEnabled(false);
        } else if (endDateEdit.date() <= startDateEdit.date()) {
            dateWarning->setText(tr("End Date must be after Start Date."));
            okButton->setEnabled(false);
        } else {
            dateWarning->setText("");
            okButton->setEnabled(true);
        }
    };
    validateForm();
    QObject::connect(&startDateEdit, &QDateEdit::dateChanged, &dialog, validateForm);
    QObject::connect(&endDateEdit, &QDateEdit::dateChanged, &dialog, validateForm);
    QObject::connect(&nameEdit, &QLineEdit::textChanged, &dialog, validateForm);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    int result = dialog.exec();
    qDebug() << "[onAddPeriod] Dialog exec result:" << result;
    if (result == QDialog::Accepted) {
        int year = yearCombo.currentText().toInt();
        QString startDateStr = startDateEdit.date().toString("yyyy-MM-dd");
        QString endDateStr = endDateEdit.date().toString("yyyy-MM-dd");
        qDebug() << "[onAddPeriod] Accepted: name=" << nameEdit.text() << ", year=" << year << ", start=" << startDateStr << ", end=" << endDateStr;
        bool added = m_dbHandler->addPeriod(nameEdit.text(), year, startDateStr, endDateStr);
        qDebug() << "[onAddPeriod] addPeriod returned:" << added;
        if (added) {
            refreshPeriodsList();
            qDebug() << "[onAddPeriod] Period list refreshed.";
        }
    } else {
        qDebug() << "[onAddPeriod] Dialog canceled.";
    }
}

void TupServerWindow::onEditPeriod() {
    int row = m_periodsTable->currentRow();
    if (row < 0) return;
    int periodId = m_periodsTable->item(row, 0)->text().toInt();
    QString name = m_periodsTable->item(row, 1)->text();
    int year = m_periodsTable->item(row, 2)->text().toInt();
    QStringList dates = m_periodsTable->item(row, 3)->text().split(" - ");
    QString start = dates.value(0), end = dates.value(1);
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Period"));
    dialog.setMinimumWidth(360);
    QFormLayout form(&dialog);
    QLineEdit nameEdit(name);
    QComboBox yearCombo;
    for (int y = 2026; y <= 2032; ++y) yearCombo.addItem(QString::number(y));
    int yearIndex = yearCombo.findText(QString::number(year));
    if (yearIndex >= 0) yearCombo.setCurrentIndex(yearIndex);
    QDateEdit startDateEdit, endDateEdit;
    startDateEdit.setCalendarPopup(true);
    endDateEdit.setCalendarPopup(true);
    startDateEdit.setDisplayFormat("yyyy-MM-dd");
    endDateEdit.setDisplayFormat("yyyy-MM-dd");
    startDateEdit.setDate(QDate::fromString(start, "yyyy-MM-dd"));
    endDateEdit.setDate(QDate::fromString(end, "yyyy-MM-dd"));
    form.addRow(tr("Name:"), &nameEdit);
    form.addRow(tr("Year:"), &yearCombo);
    form.addRow(tr("Start Date:"), &startDateEdit);
    form.addRow(tr("End Date:"), &endDateEdit);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form.addRow(&buttons);
    QPushButton *okButton = buttons.button(QDialogButtonBox::Ok);
    QLabel *dateWarning = new QLabel;
    dateWarning->setStyleSheet("color: red");
    form.addRow("", dateWarning);
    auto validateForm = [&]() {
        if (nameEdit.text().trimmed().isEmpty()) {
            dateWarning->setText(tr("Name cannot be empty."));
            okButton->setEnabled(false);
        } else if (endDateEdit.date() <= startDateEdit.date()) {
            dateWarning->setText(tr("End Date must be after Start Date."));
            okButton->setEnabled(false);
        } else {
            dateWarning->setText("");
            okButton->setEnabled(true);
        }
    };
    validateForm();
    QObject::connect(&startDateEdit, &QDateEdit::dateChanged, &dialog, validateForm);
    QObject::connect(&endDateEdit, &QDateEdit::dateChanged, &dialog, validateForm);
    QObject::connect(&nameEdit, &QLineEdit::textChanged, &dialog, validateForm);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        int newYear = yearCombo.currentText().toInt();
        QString startDateStr = startDateEdit.date().toString("yyyy-MM-dd");
        QString endDateStr = endDateEdit.date().toString("yyyy-MM-dd");
        if (m_dbHandler->updatePeriod(periodId, nameEdit.text(), newYear, startDateStr, endDateStr)) {
            refreshPeriodsList();
        }
    }
}

void TupServerWindow::onRemovePeriod() {
    int row = m_periodsTable->currentRow();
    if (row < 0) return;
    int periodId = m_periodsTable->item(row, 0)->text().toInt();
    // Check for projects linked to this period
    bool hasProjects = false;
    QList<DatabaseHandler::ProjectRecord> projects = m_dbHandler->getAllProjects();
    for (const auto &project : projects) {
        if (project.periodId == periodId) {
            hasProjects = true;
            break;
        }
    }
    if (hasProjects) {
        QMessageBox::warning(this, tr("Remove Period"), tr("This period cannot be removed because it has associated projects."));
        return;
    }
    if (QMessageBox::question(this, tr("Remove Period"), tr("Are you sure?")) == QMessageBox::Yes) {
        if (m_dbHandler->removePeriod(periodId)) {
            refreshPeriodsList();
        }
    }
}

// --- Stub implementations to resolve linker errors ---
void TupServerWindow::onAddClass() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Class"));
    dialog.setMinimumWidth(360);
    QFormLayout form(&dialog);
    QLineEdit nameEdit, descEdit;
    QComboBox yearCombo;
    for (int y = 2026; y <= 2032; ++y) yearCombo.addItem(QString::number(y));
    form.addRow(tr("Name:"), &nameEdit);
    form.addRow(tr("Year:"), &yearCombo);
    form.addRow(tr("Description:"), &descEdit);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form.addRow(&buttons);
    QPushButton *okButton = buttons.button(QDialogButtonBox::Ok);
    QLabel *warningLabel = new QLabel;
    warningLabel->setStyleSheet("color: red");
    form.addRow("", warningLabel);
    auto validateForm = [&]() {
        QString name = nameEdit.text().trimmed();
        if (name.isEmpty()) {
            warningLabel->setText(tr("Class name cannot be empty."));
            okButton->setEnabled(false);
        } else {
            warningLabel->setText("");
            okButton->setEnabled(true);
        }
    };
    validateForm();
    QObject::connect(&nameEdit, &QLineEdit::textChanged, &dialog, validateForm);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit.text().trimmed();
        int year = yearCombo.currentText().toInt();
        if (m_dbHandler->addClass(name, year, descEdit.text().trimmed())) {
            refreshClassesList();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to add class. It may already exist or the input is invalid."));
        }
    }
}
void TupServerWindow::refreshPeriodsList() {
    m_periodsTable->setRowCount(0);
    QList<DatabaseHandler::PeriodInfo> periods = m_dbHandler->getAllPeriods();
    for (const auto &p : periods) {
        int row = m_periodsTable->rowCount();
        m_periodsTable->insertRow(row);
        m_periodsTable->setItem(row, 0, new QTableWidgetItem(QString::number(p.periodId)));
        m_periodsTable->setItem(row, 1, new QTableWidgetItem(p.name));
        m_periodsTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.year)));
        m_periodsTable->setItem(row, 3, new QTableWidgetItem(p.startDate + " - " + p.endDate));
    }
}

// === Render / Watch / Grade slots ===

void TupServerWindow::renderProject()
{
    int row = m_projectsTable->currentRow();
    if (row < 0) return;

    int projectId = m_projectsTable->item(row, 0)->text().toInt();
    QString title  = m_projectsTable->item(row, 1)->text();

    QProgressDialog progress(tr("Rendering \"%1\"...").arg(title), QString(), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.show();
    QApplication::processEvents();

    ProjectRenderer::RenderResult result = m_projectRenderer->renderProject(projectId);
    progress.close();

    if (result.success) {
        appendLog(tr("Project '%1' rendered successfully: %2").arg(title, result.mp4Path), "INFO");
        refreshProjectsList(m_projectFilterEdit ? m_projectFilterEdit->text() : QString());
        m_watchProjectButton->setEnabled(true);
        QMessageBox::information(this, tr("Render Complete"),
            tr("Project \"%1\" rendered successfully.\n\nFile: %2").arg(title, result.mp4Path));
    } else {
        appendLog(tr("Render failed for '%1': %2").arg(title, result.errorMessage), "ERROR");
        QMessageBox::critical(this, tr("Render Failed"),
            tr("Failed to render \"%1\":\n\n%2").arg(title, result.errorMessage));
    }
}

void TupServerWindow::watchProject()
{
    int row = m_projectsTable->currentRow();
    if (row < 0) return;

    int projectId = m_projectsTable->item(row, 0)->text().toInt();
    DatabaseHandler::RenderProjectInfo info = m_dbHandler->getProjectRenderInfo(projectId);
    if (!info.found) {
        QMessageBox::warning(this, tr("Error"), tr("Could not retrieve project information."));
        return;
    }

    TCONFIG->beginGroup("Render");
    QString renderPath = TCONFIG->value("RenderPath").toString();
    TCONFIG->endGroup();

    QString mp4Path = renderPath + "/" + QString::number(info.studentId) + "/" + info.filename + ".mp4";
    if (!QFile::exists(mp4Path)) {
        QMessageBox::warning(this, tr("File Not Found"),
            tr("MP4 file not found:\n%1\n\nPlease render the project first.").arg(mp4Path));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(mp4Path));
}

void TupServerWindow::gradeProject()
{
    int row = m_projectsTable->currentRow();
    if (row < 0) return;

    int projectId = m_projectsTable->item(row, 0)->text().toInt();
    QString title = m_projectsTable->item(row, 1)->text();

    // Retrieve full project record to get ownerId, periodId, classId
    QList<DatabaseHandler::ProjectRecord> projects = m_dbHandler->getAllProjects();
    DatabaseHandler::ProjectRecord record;
    bool found = false;
    for (const auto &p : projects) {
        if (p.projectId == projectId) {
            record = p;
            found = true;
            break;
        }
    }
    if (!found) {
        QMessageBox::warning(this, tr("Error"), tr("Could not retrieve project data."));
        return;
    }

    // Load existing grade if any
    DatabaseHandler::GradeInfo existing = m_dbHandler->getGrade(projectId, record.ownerId);

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Grade Project: %1").arg(title));
    dialog.setMinimumWidth(380);

    QFormLayout *form = new QFormLayout(&dialog);

    QLabel *ownerLabel = new QLabel(record.ownerStudentname);
    form->addRow(tr("Student:"), ownerLabel);

    QDoubleSpinBox *gradeSpinBox = new QDoubleSpinBox();
    gradeSpinBox->setRange(0.0, 10.0);
    gradeSpinBox->setSingleStep(0.5);
    gradeSpinBox->setDecimals(1);
    gradeSpinBox->setValue(existing.found ? existing.grade : 0.0);
    form->addRow(tr("Grade (0–10):"), gradeSpinBox);

    QTextEdit *commentsEdit = new QTextEdit();
    commentsEdit->setPlaceholderText(tr("Optional comments..."));
    commentsEdit->setMaximumHeight(100);
    if (existing.found)
        commentsEdit->setPlainText(existing.comments);
    form->addRow(tr("Comments:"), commentsEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    double grade    = gradeSpinBox->value();
    QString comments = commentsEdit->toPlainText().trimmed();

    // teacherStudentId: 0 means the server/admin teacher (can be updated later)
    bool ok = m_dbHandler->saveGrade(projectId, record.ownerId, 0,
                                     record.periodId, record.classId,
                                     grade, comments);
    if (ok) {
        appendLog(tr("Grade %.1f saved for project '%2' (student: %3)")
                      .arg(grade).arg(title).arg(record.ownerStudentname), "INFO");
        refreshProjectsList(m_projectFilterEdit ? m_projectFilterEdit->text() : QString());
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save grade."));
        appendLog(tr("Failed to save grade for project '%1'").arg(title), "ERROR");
    }
}
