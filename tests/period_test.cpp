#include "period_test.h"
#include "testhelpers.h"
#include "../src/server/modules/projects/databasehandler.h"
#include <algorithm>

void PeriodTest::initTestCase() { initTestDatabase(); }
void PeriodTest::cleanup()      { cleanupDatabase(); }

void PeriodTest::test_crud()
{
    DatabaseHandler db;
    QVERIFY(db.addPeriod("CrudPeriod", 2026, "2026-01-01", "2026-12-31"));
    auto periods = db.getAllPeriods();
    auto it = std::find_if(periods.begin(), periods.end(),
        [](const DatabaseHandler::PeriodInfo &p){ return p.name == "CrudPeriod"; });
    QVERIFY(it != periods.end());
    int id = it->periodId;

    QVERIFY(db.updatePeriod(id, "CrudPeriod2", 2027, "2027-01-01", "2027-12-31"));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(),
        [id](const DatabaseHandler::PeriodInfo &p){
            return p.periodId == id && p.name == "CrudPeriod2";
        });
    QVERIFY(it != periods.end());

    QVERIFY(db.removePeriod(id));
    periods = db.getAllPeriods();
    it = std::find_if(periods.begin(), periods.end(),
        [id](const DatabaseHandler::PeriodInfo &p){ return p.periodId == id; });
    QVERIFY(it == periods.end());
}

void PeriodTest::test_add()
{
    DatabaseHandler db;
    QVERIFY(db.addPeriod("AddPeriod", 2026, "2026-01-01", "2026-12-31"));
    const auto periods = db.getAllPeriods();
    QVERIFY(std::any_of(periods.begin(), periods.end(),
        [](const DatabaseHandler::PeriodInfo &p){ return p.name == "AddPeriod"; }));
}

void PeriodTest::test_update()
{
    DatabaseHandler db;
    int id = makePeriod(db, "BeforeUpdate");
    QVERIFY(id > 0);
    QVERIFY(db.updatePeriod(id, "AfterUpdate", 2027, "2027-01-01", "2027-12-31"));
    const auto periods = db.getAllPeriods();
    QVERIFY(std::any_of(periods.begin(), periods.end(),
        [id](const DatabaseHandler::PeriodInfo &p){
            return p.periodId == id && p.name == "AfterUpdate";
        }));
}

void PeriodTest::test_remove()
{
    DatabaseHandler db;
    int id = makePeriod(db, "ToRemovePeriod");
    QVERIFY(id > 0);
    QVERIFY(db.removePeriod(id));
    const auto periods = db.getAllPeriods();
    QVERIFY(std::none_of(periods.begin(), periods.end(),
        [id](const DatabaseHandler::PeriodInfo &p){ return p.periodId == id; }));
}
