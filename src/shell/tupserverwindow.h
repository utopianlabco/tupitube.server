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
#ifndef TUPSERVERWINDOW_H
#define TUPSERVERWINDOW_H

#include "server.h"
#include "databasehandler.h"

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCloseEvent>
#include <QMenuBar>
#include <QComboBox>
#include <QDialog>
#include <QCheckBox>
#include <QFormLayout>
#include <QDialogButtonBox>

class TupServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    TupServerWindow(QWidget *parent = nullptr);
    ~TupServerWindow();
    void startServer();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void toggleServer();
    void onServerStarted();
    void onServerStopped();
    void onConnectionCountChanged(int count);
    void onStudentConnected(const QString &studentName, const QString &ip);
    void onStudentDisconnected(const QString &studentName);
    void onLogMessage(const QString &message, const QString &level);
    void saveSettings();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void clearLogs();
    void saveConfigSettings(); // Added method declaration

    // Student management slots
    void refreshStudentsList(const QString &filter = QString());
    void addStudent();
    void editStudent();
    void removeStudent();
    void importUsersFromCsv();

    // Project collaboration slots
    void refreshProjectsList();
    void createProject();
    void manageCollaborators();
    void removeProject();
    void onProjectSelectionChanged();
    void viewProjectChat();
    void sendBroadcastMessage();

    // Classes tab slots
    void refreshClassesList();
    void refreshPeriodsList();
    void onAddClass();
    void onEditClass();
    void onRemoveClass();
    void onAddPeriod();
    void onEditPeriod();
    void onRemovePeriod();

private:
    void setupClassesTab();
    void setupUI();
    void setupMenuBar();
    void setupTabs();
    void setupStatusTab();
    void setupStudentsTab();
    void setupProjectsTab();
    void setupLogsTab();
    void setupSettingsTab();
    void setupTrayIcon();
    void loadSettings();
    void updateDerivedPaths();
    void appendLog(const QString &message, const QString &level);
    void updateTrayTooltip();
    void updateCollaboratorsDisplay(int projectId);

    // Server
    TcpServer *m_server;
    bool m_serverRunning;

    // Tabs
    QTabWidget *m_tabWidget;
    QWidget *m_statusTab;
    QWidget *m_studentsTab;
    QWidget *m_projectsTab;
    QWidget *m_logsTab;
    QWidget *m_classesTab;
    QWidget *m_settingsTab;
    // Classes tab widgets
    QTableWidget *m_classesTable;
    QTableWidget *m_periodsTable;
    QPushButton *m_addClassButton;
    QPushButton *m_editClassButton;
    QPushButton *m_removeClassButton;
    QPushButton *m_addPeriodButton;
    QPushButton *m_editPeriodButton;
    QPushButton *m_removePeriodButton;
    QPushButton *m_refreshClassesButton;

    // Status tab widgets
    QPushButton *m_toggleButton;
    QPushButton *m_broadcastButton;
    QLabel *m_statusLabel;
    QLabel *m_hostLabel;
    QLabel *m_portLabel;
    QLabel *m_connectionCountLabel;
    QLabel *m_uptimeLabel;

    // Students tab widgets
    QTableWidget *m_connectedStudentsTable;
    QTableWidget *m_registeredStudentsTable;
    QPushButton *m_addStudentButton;
    QPushButton *m_editStudentButton;
    QPushButton *m_removeStudentButton;
    QPushButton *m_refreshStudentsButton;
    QPushButton *m_importCsvButton;
    QLineEdit *m_studentFilterEdit; // Filter for Registered Students
    DatabaseHandler *m_dbHandler;

    // Projects tab widgets
    QTableWidget *m_projectsTable;
    QTableWidget *m_collaboratorsTable;
    QPushButton *m_createProjectButton;
    QPushButton *m_manageCollaboratorsButton;
    QPushButton *m_viewChatButton;
    QPushButton *m_removeProjectButton;
    QPushButton *m_refreshProjectsButton;

    // Logs tab widgets
    QTextEdit *m_logView;
    QPushButton *m_clearLogsButton;

    // Settings tab widgets
    QComboBox *m_hostCombo;
    QSpinBox *m_portSpin;
    QLineEdit *m_dataPathEdit;
    QLabel *m_databasePathLabel;
    QLabel *m_cachePathLabel;
    QLabel *m_projectsPathLabel;
    QComboBox *m_languageCombo;
    QComboBox *m_themeCombo;
    QPushButton *m_saveSettingsButton;
    QPushButton *m_browseDataPathButton;

    // System tray
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

    // Menu bar
    QAction *m_toggleServerAction;

    // State
    QDateTime m_startTime;
    QTimer *m_uptimeTimer;
};

#endif // TUPSERVERWINDOW_H
