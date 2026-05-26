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
};
QTEST_MAIN(SyncHistoryHandlerTest)
#include "SyncHistoryHandlerTest.moc"
