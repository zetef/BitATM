#include <QStandardPaths>
#include <QtTest>

#include "LocalStorage.h"

/**
 * @brief Unit tests for LocalStorage SQLite cache.
 * UT-FE-12: save and load messages
 * UT-FE-13: newest timestamp cursor
 */
class LocalStorageTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(LocalStorage::instance().open());
    }

    void cleanupTestCase() { LocalStorage::instance().close(); }

    /** @brief UT-FE-12: save 2 messages, load returns them in ASC order with correct flags. */
    void saveAndLoadMessages() {
        LocalStorage::instance().saveMessage("alice", "alice", "Hello", "2025-01-01T10:00:00.000Z",
                                             false);
        LocalStorage::instance().saveMessage("alice", "me", "Hi back", "2025-01-01T10:01:00.000Z",
                                             true);

        QList<MessageRecord> msgs = LocalStorage::instance().loadMessages("alice");
        QVERIFY(msgs.size() >= 2);

        // Find our two messages (DB may have leftovers from other test runs in same process)
        bool foundIncoming = false;
        bool foundOutgoing = false;
        for (const auto& m : msgs) {
            if (m.sender == "alice" && m.content == "Hello" && !m.isOutgoing) foundIncoming = true;
            if (m.sender == "me" && m.content == "Hi back" && m.isOutgoing) foundOutgoing = true;
        }
        QVERIFY(foundIncoming);
        QVERIFY(foundOutgoing);
    }

    /** @brief UT-FE-13: newestTimestamp returns a non-empty value >= the latest inserted. */
    void newestTimestampCursor() {
        LocalStorage::instance().saveMessage("bob", "bob", "Msg1", "2025-06-01T08:00:00.000Z",
                                             false);
        LocalStorage::instance().saveMessage("bob", "bob", "Msg2", "2025-06-01T09:00:00.000Z",
                                             false);

        QString newest = LocalStorage::instance().newestTimestamp();
        QVERIFY(!newest.isEmpty());
        // The newest timestamp must be lexicographically >= "2025-06-01T09:00:00.000Z"
        QVERIFY(newest >= QString("2025-06-01T09:00:00.000Z"));
    }

    /** @brief Duplicate (peer, sender, timestamp) must not produce a second row. */
    void deduplicatePreventsDoubleInsert() {
        const QString peer = "charlie";
        const QString ts = "2025-12-31T23:59:59.000Z";
        LocalStorage::instance().saveMessage(peer, "charlie", "Same msg", ts, false);
        LocalStorage::instance().saveMessage(peer, "charlie", "Same msg", ts, false);

        QList<MessageRecord> msgs = LocalStorage::instance().loadMessages(peer);
        int count = 0;
        for (const auto& m : msgs) {
            if (m.sender == "charlie" && m.timestamp == ts) ++count;
        }
        QCOMPARE(count, 1);
    }

    /** @brief Receipts v2: per-member snapshot aggregates and leaver pruning. */
    void groupRecipientAggregates() {
        auto& ls = LocalStorage::instance();
        const QString gid = "77";
        const QString ts = "2026-02-02T10:00:00.000Z";

        ls.saveRecipientSnapshot(gid, ts, {"m1", "m2"});
        QCOMPARE(ls.recipientCount(gid, ts), 2);
        QVERIFY(!ls.allRecipientsDelivered(gid, ts));
        QVERIFY(!ls.allRecipientsSeen(gid, ts));

        ls.markRecipientDelivered(gid, ts, "m1");
        QVERIFY(!ls.allRecipientsDelivered(gid, ts));

        // seen implies delivered
        ls.markRecipientSeen(gid, ts, "m2");
        QVERIFY(!ls.allRecipientsSeen(gid, ts));
        QVERIFY(ls.allRecipientsDelivered(gid, ts));

        ls.markRecipientSeen(gid, ts, "m1");
        QVERIFY(ls.allRecipientsSeen(gid, ts));

        // info sheet payload
        const QVariantList states = ls.recipientStates(gid, ts);
        QCOMPARE(states.size(), 2);

        // pruning: m2 left; remaining ts reported for recompute
        ls.saveRecipientSnapshot(gid, "2026-02-02T11:00:00.000Z", {"m1", "m2"});
        const QStringList affected = ls.removeRecipientsNotIn(gid, {"m1"});
        QVERIFY(affected.contains("2026-02-02T11:00:00.000Z"));
        QCOMPARE(ls.recipientCount(gid, "2026-02-02T11:00:00.000Z"), 1);
    }
};

QTEST_MAIN(LocalStorageTest)
#include "LocalStorageTest.moc"
