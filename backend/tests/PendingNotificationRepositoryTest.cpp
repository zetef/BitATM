#include <QtTest>

#include "DbManager.h"
#include "PendingNotificationRepository.h"

class PendingNotificationRepositoryTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        DbManager::instance().init(qgetenv("DATABASE_URL").toStdString());
    }

    // queue -> findAndClear returns it once, second call returns empty
    void queueThenFindAndClearIsConsumeOnce() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        PendingNotificationRepository repo;
        repo.queue("ut_pending_recipient", PacketType::DELETE_CONVERSATION, "ut_pending_actor", "");

        auto first = repo.findAndClear("ut_pending_recipient");
        QCOMPARE(first.size(), static_cast<size_t>(1));
        QCOMPARE(static_cast<int>(first[0].type),
                 static_cast<int>(PacketType::DELETE_CONVERSATION));
        QCOMPARE(first[0].fromUser, std::string("ut_pending_actor"));

        auto second = repo.findAndClear("ut_pending_recipient");
        QVERIFY(second.empty());
    }

    // body field (e.g. group id) round-trips
    void bodyFieldRoundTrips() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        PendingNotificationRepository repo;
        repo.queue("ut_pending_recipient2", PacketType::GROUP_LEAVE, "ut_pending_creator", "42");

        auto rows = repo.findAndClear("ut_pending_recipient2");
        QCOMPARE(rows.size(), static_cast<size_t>(1));
        QCOMPARE(rows[0].body, std::string("42"));
    }
};
QTEST_MAIN(PendingNotificationRepositoryTest)
#include "PendingNotificationRepositoryTest.moc"
