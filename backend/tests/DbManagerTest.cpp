#include <QtTest>

class DbManagerTest : public QObject {
    Q_OBJECT
private slots:
    void insertUser() { QFAIL("not implemented"); }
    void storeOfflineMessage() { QFAIL("not implemented"); }
};

QTEST_MAIN(DbManagerTest)
#include "DbManagerTest.moc"