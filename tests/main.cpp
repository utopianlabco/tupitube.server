#include <QApplication>
#include <QTest>
#include <QLoggingCategory>
#include "../../tupitube.desk/src/framework/core/tapplicationproperties.h"

#include "class_test.h"
#include "period_test.h"
#include "student_test.h"
#include "project_test.h"
#include "chat_test.h"
#include "grade_test.h"
#include "settings_test.h"
#include "tupserverwindow_existence_test.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("tupitube_server");

    // Silence debug/warning noise from production code (plugin loading, DB
    // init, logger startup, etc.) so test output stays clean.
    QLoggingCategory::setFilterRules("*.debug=false\n*.warning=false");

    TApplicationProperties::instance();
#ifdef TEST_PLUGINS_PATH
    kAppProp->setPluginDir(QString(TEST_PLUGINS_PATH));
#endif

    int status = 0;
    auto run = [&](QObject *test) {
        status |= QTest::qExec(test, argc, argv);
        delete test;
    };

    run(new ClassTest);
    run(new PeriodTest);
    run(new StudentTest);
    run(new ProjectTest);
    run(new ChatTest);
    run(new GradeTest);
    run(new SettingsTest);
    run(new TupServerWindowExistenceTest);

    return status;
}

