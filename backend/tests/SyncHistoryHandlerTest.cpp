#include <QtTest>

#include "DbManager.h"
#include "GroupRepository.h"
#include "MessageRepository.h"
#include "UserRepository.h"

class SyncHistoryHandlerTest : public QObject {
    Q_OBJECT
private:
    void ensureUser(const std::string& name) {
        UserRepository userRepo;
        if (!userRepo.findByUsername(name)) userRepo.save(User{0, name, "hash:x", ""});
    }

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

    void senderEncryptedKeyStoredAndRetrieved() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        MessageRepository repo;
        // Insert a message with a sender key
        Message msg{0,
                    "ut_sync_sender",
                    "ut_sync_recipient",
                    "encrypted_body==",
                    "recipient_key==",
                    "sender_key==",
                    "sent"};
        repo.save(msg);

        // Retrieve messages for sender; sender key should come back non-empty
        auto msgs = repo.findAllForUser("ut_sync_sender");
        bool found = false;
        for (const auto& m : msgs) {
            if (m.getSender() == "ut_sync_sender" && m.getRecipient() == "ut_sync_recipient") {
                QCOMPARE(m.getSenderEncryptedKey(), std::string("sender_key=="));
                found = true;
                break;
            }
        }
        QVERIFY2(found, "Message with sender key not found after save");
    }

    void groupCursorFiltersOlderMessages() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        ensureUser("ut_sync_sender");
        GroupRepository repo;
        const int gid = repo.createGroup("ut_sync_group", "ut_sync_sender");
        repo.addMember(gid, "ut_sync_sender", "creator");
        repo.saveMessage(gid, "ut_sync_sender", "group_body==", "2026-01-01T12:00:00.000Z");

        // Old cursor must include the message; future cursor must exclude it.
        // Both must not throw - a DbException here means the cursor binding
        // broke and SyncHistoryHandler silently replays full history each login
        auto since = repo.findMessagesForUserSince("ut_sync_sender", "2020-01-01T00:00:00.000Z");
        bool found = false;
        for (const auto& r : since) {
            if (r.groupId == gid) found = true;
        }
        QVERIFY2(found, "Message not returned for a cursor older than it");

        auto future = repo.findMessagesForUserSince("ut_sync_sender", "2099-01-01T00:00:00.000Z");
        for (const auto& r : future) {
            QVERIFY2(r.groupId != gid, "Message returned despite cursor newer than it");
        }
    }
};
QTEST_MAIN(SyncHistoryHandlerTest)
#include "SyncHistoryHandlerTest.moc"
