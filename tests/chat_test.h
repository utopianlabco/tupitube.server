#ifndef CHAT_TEST_H
#define CHAT_TEST_H
#include <QtTest/QtTest>

class ChatTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void test_saveMessage();
    void test_getHistory();
    void test_getHistoryByDate();
    void test_clearHistory();
};
#endif // CHAT_TEST_H
