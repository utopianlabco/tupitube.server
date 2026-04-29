#include <QApplication>
#include <QTest>
#include "tupitube_server_feature_test.cpp"
#include "tupserverwindow_existence_test.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    int status = 0;
    {
        TupitubeServerFeatureTest featureTest;
        status |= QTest::qExec(&featureTest, argc, argv);
    }
    {
        TupServerWindowExistenceTest windowTest;
        status |= QTest::qExec(&windowTest, argc, argv);
    }
    return status;
}
