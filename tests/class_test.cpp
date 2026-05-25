#include "class_test.h"
#include "testhelpers.h"
#include "../src/server/modules/projects/databasehandler.h"
#include <algorithm>

void ClassTest::initTestCase() { initTestDatabase(); }
void ClassTest::cleanup()      { cleanupDatabase(); }

void ClassTest::test_crud()
{
    DatabaseHandler db;
    QVERIFY(db.addClass("CrudClass", 2026, "desc"));
    auto classes = db.getAllClasses();
    auto it = std::find_if(classes.begin(), classes.end(),
        [](const DatabaseHandler::ClassInfo &c){ return c.name == "CrudClass"; });
    QVERIFY(it != classes.end());
    int id = it->classId;

    QVERIFY(db.updateClass(id, "CrudClass2", 2027, "desc2"));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(),
        [id](const DatabaseHandler::ClassInfo &c){ return c.classId == id && c.name == "CrudClass2"; });
    QVERIFY(it != classes.end());

    QVERIFY(db.removeClass(id));
    classes = db.getAllClasses();
    it = std::find_if(classes.begin(), classes.end(),
        [id](const DatabaseHandler::ClassInfo &c){ return c.classId == id; });
    QVERIFY(it == classes.end());
}

void ClassTest::test_add()
{
    DatabaseHandler db;
    QVERIFY(db.addClass("AddClass", 2026, "desc"));
    const auto classes = db.getAllClasses();
    QVERIFY(std::any_of(classes.begin(), classes.end(),
        [](const DatabaseHandler::ClassInfo &c){ return c.name == "AddClass"; }));
}

void ClassTest::test_update()
{
    DatabaseHandler db;
    int id = makeClass(db, "BeforeUpdate");
    QVERIFY(id > 0);
    QVERIFY(db.updateClass(id, "AfterUpdate", 2027, "new desc"));
    const auto classes = db.getAllClasses();
    QVERIFY(std::any_of(classes.begin(), classes.end(),
        [id](const DatabaseHandler::ClassInfo &c){
            return c.classId == id && c.name == "AfterUpdate";
        }));
}

void ClassTest::test_remove()
{
    DatabaseHandler db;
    int id = makeClass(db, "ToRemove");
    QVERIFY(id > 0);
    QVERIFY(db.removeClass(id));
    const auto classes = db.getAllClasses();
    QVERIFY(std::none_of(classes.begin(), classes.end(),
        [id](const DatabaseHandler::ClassInfo &c){ return c.classId == id; }));
}
