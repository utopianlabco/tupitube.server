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

// UI setup methods for TupServerWindow.
// All widget construction and layout code lives here.
// Runtime logic (server control, slots, dialogs) is in tupserverwindow.cpp.

#include "tupserverwindow.h"
#include "tservertheme.h"

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
#include <QDir>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QListWidget>

#include <QLineEdit>
#include <QTableWidgetItem>
#include <QStringList>

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
    connect(m_projectsTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        if (item->column() == 1)
            renameProject();
        else if (item->column() == 3)
            manageCollaborators();
        else if (item->column() == 5)
            gradeProject();
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

    m_playProjectButton = new QPushButton(tr("Play"));
    m_playProjectButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playProjectButton->setEnabled(false);
    m_playProjectButton->setToolTip(tr("Render if needed and play the project MP4"));
    connect(m_playProjectButton, &QPushButton::clicked, this, &TupServerWindow::playProject);
    projectButtonLayout->addWidget(m_playProjectButton);

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
    m_collaboratorsTable->setHorizontalHeaderLabels({tr("Username"), tr("Full Name"), tr("Permission")});
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
