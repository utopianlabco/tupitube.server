#ifndef TUPSERVERWINDOW_EXISTENCE_TEST_H
#define TUPSERVERWINDOW_EXISTENCE_TEST_H
#include <QtTest/QtTest>
#include "../src/shell/tupserverwindow.h"
class TupServerWindowExistenceTest : public QObject {
    Q_OBJECT
private slots:
    void test_window_creation();
    void test_tabs_and_widgets_exist();
    void test_menu_and_actions_exist();
    void test_tab_names();
    void test_table_count();
    void test_gradebook_button_exists();
};
#endif // TUPSERVERWINDOW_EXISTENCE_TEST_H