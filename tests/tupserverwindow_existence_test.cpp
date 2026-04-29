#include "tupserverwindow_existence_test.h"

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
