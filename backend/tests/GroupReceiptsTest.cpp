#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include <QtTest>

#include "DbManager.h"
#include "GroupRepository.h"
#include "UserRepository.h"

class GroupReceiptsTest : public QObject {
    Q_OBJECT
private:
    void ensureUser(const std::string& name) {
        UserRepository userRepo;
        if (!userRepo.findByUsername(name)) userRepo.save(User{0, name, "hash:x", ""});
    }

    // Receipt row state for (messageId, member): "none", "pending", "delivered", "seen"
    std::string rowState(int messageId, const std::string& member) {
        auto ses = DbManager::instance().session();
        using namespace Poco::Data::Keywords;
        int mid = messageId;
        std::string user = member;
        int delivered = 0, seen = 0, found = 0;
        // aggregate form always returns exactly one row, even when no match
        ses << "SELECT COUNT(*)::int, "
               "COALESCE(MAX((delivered_at IS NOT NULL)::int), 0), "
               "COALESCE(MAX((seen_at IS NOT NULL)::int), 0) "
               "FROM group_message_receipts WHERE message_id = $1 AND username = $2",
            into(found), into(delivered), into(seen), use(mid), use(user), now;
        if (!found) return "none";
        if (seen) return "seen";
        if (delivered) return "delivered";
        return "pending";
    }

private slots:
    void initTestCase() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set - skipping DB tests");
        DbManager::instance().init(qgetenv("DATABASE_URL").toStdString());
        ensureUser("ut_rcpt_sender");
        ensureUser("ut_rcpt_m1");
        ensureUser("ut_rcpt_m2");
    }

    void snapshotSeenAllAndPruning() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        GroupRepository repo;
        const int gid = repo.createGroup("ut_rcpt_group", "ut_rcpt_sender");
        repo.addMember(gid, "ut_rcpt_sender", "creator");
        repo.addMember(gid, "ut_rcpt_m1", "member");
        repo.addMember(gid, "ut_rcpt_m2", "member");
        const std::string ts = "2026-02-02T10:00:00.000Z";
        const int msgId = repo.saveMessage(gid, "ut_rcpt_sender", "body==", ts);

        // snapshot rows are exactly what the caller passes (recipients)
        repo.insertReceiptRows(msgId, {"ut_rcpt_m1", "ut_rcpt_m2"});
        QCOMPARE(rowState(msgId, "ut_rcpt_m1"), std::string("pending"));
        QCOMPARE(rowState(msgId, "ut_rcpt_m2"), std::string("pending"));
        QCOMPARE(rowState(msgId, "ut_rcpt_sender"), std::string("none"));

        // delivered marks only that member; returns the message sender
        auto senders = repo.markReceiptDelivered(gid, ts, "ut_rcpt_m1");
        QCOMPARE(senders.size(), static_cast<size_t>(1));
        QCOMPARE(senders[0], std::string("ut_rcpt_sender"));
        QCOMPARE(rowState(msgId, "ut_rcpt_m1"), std::string("delivered"));
        QCOMPARE(rowState(msgId, "ut_rcpt_m2"), std::string("pending"));

        // seen implies delivered (m2 never sent a delivered ACK)
        repo.markReceiptSeen(gid, ts, "ut_rcpt_m2");
        QCOMPARE(rowState(msgId, "ut_rcpt_m2"), std::string("seen"));

        // idempotent re-mark
        repo.markReceiptDelivered(gid, ts, "ut_rcpt_m1");
        QCOMPARE(rowState(msgId, "ut_rcpt_m1"), std::string("delivered"));

        // pruning a leaver removes their rows only
        repo.deleteReceiptsForMember(gid, "ut_rcpt_m1");
        QCOMPARE(rowState(msgId, "ut_rcpt_m1"), std::string("none"));
        QCOMPARE(rowState(msgId, "ut_rcpt_m2"), std::string("seen"));
    }
};
QTEST_MAIN(GroupReceiptsTest)
#include "GroupReceiptsTest.moc"
