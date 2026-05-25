#include "tupserverwindow_existence_test.h"
#include <algorithm>

void TupServerWindowExistenceTest::test_window_creation() {
	TupServerWindow w;
	QVERIFY(w.windowTitle().isEmpty() || !w.windowTitle().isNull());
}

void TupServerWindowExistenceTest::test_tabs_and_widgets_exist() {
	TupServerWindow w;
	// Tabs
	QVERIFY(w.findChild<QTabWidget*>());
	// Main tab widgets
	QVERIFY(w.findChild<QTableWidget*>());
	QVERIFY(w.findChild<QPushButton*>());
	QVERIFY(w.findChild<QLabel*>());
	QVERIFY(w.findChild<QLineEdit*>());
	QVERIFY(w.findChild<QComboBox*>());
	QVERIFY(w.findChild<QSpinBox*>());
	QVERIFY(w.findChild<QTextEdit*>());
}

void TupServerWindowExistenceTest::test_menu_and_actions_exist() {
	TupServerWindow w;
	QMenuBar* menuBar = w.menuBar();
	QVERIFY(menuBar);
	QList<QMenu*> menus = menuBar->findChildren<QMenu*>();
	QVERIFY(!menus.isEmpty());
	QList<QAction*> actions = menuBar->actions();
	QVERIFY(!actions.isEmpty());
}

void TupServerWindowExistenceTest::test_tab_names() {
	TupServerWindow w;
	QTabWidget *tabs = w.findChild<QTabWidget*>();
	QVERIFY(tabs);
	QCOMPARE(tabs->count(), 6);
	const QStringList expected = {"Status", "Logs", "Classes", "Students", "Projects", "Settings"};
	for (const QString &name : expected) {
		bool found = false;
		for (int i = 0; i < tabs->count(); i++) {
			if (tabs->tabText(i) == name) { found = true; break; }
		}
		QVERIFY2(found, qPrintable("Missing tab: " + name));
	}
}

void TupServerWindowExistenceTest::test_table_count() {
	TupServerWindow w;
	// classesTable, periodsTable, connectedStudentsTable,
	// registeredStudentsTable, projectsTable, collaboratorsTable
	auto tables = w.findChildren<QTableWidget*>();
	QVERIFY2(tables.size() >= 6,
	         qPrintable(QString("Expected >= 6 tables, found %1").arg(tables.size())));
}

void TupServerWindowExistenceTest::test_gradebook_button_exists() {
	TupServerWindow w;
	auto buttons = w.findChildren<QPushButton*>();
	bool found = std::any_of(buttons.begin(), buttons.end(),
	    [](QPushButton *b){ return b->text() == "Grade Book"; });
	QVERIFY2(found, "Grade Book button not found in TupServerWindow");
}
