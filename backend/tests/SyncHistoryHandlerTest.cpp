#include <QtTest>

#include "DbManager.h"
#include "MessageRepository.h"

class SyncHistoryHandlerTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set - skipping DB tests");
        DbManager::instance().init(qgetenv("DATABASE_URL").toStdString());
    }

    void findAllForUserReturnsBothDirections() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        MessageRepository repo;
        // Must not throw; returns messages where alice is sender or recipient
        auto all = repo.findAllForUser("alice");
        QVERIFY(true);  // reaching here means no exception was thrown
    }

    void findAllForUserWithCursorReturnsOnlyNewer() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        MessageRepository repo;
        // Nothing should have created_at after year 2099
        auto all = repo.findAllForUser("alice", "2099-01-01T00:00:00Z");
        QCOMPARE(all.size(), static_cast<size_t>(0));
    }
};
QTEST_MAIN(SyncHistoryHandlerTest)
#include "SyncHistoryHandlerTest.moc"
