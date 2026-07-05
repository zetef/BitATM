#include <QtTest>
#include <cstdlib>

#include "DbManager.h"
#include "Message.h"
#include "MessageRepository.h"
#include "OfflineMessage.h"
#include "OfflineQueueRepository.h"
#include "User.h"
#include "UserRepository.h"

class OfflineQueueRepositoryTest : public QObject {
    Q_OBJECT
private:
    bool _dbAvailable = false;

    // Create user pair + message + undelivered offline entry; returns entry id.
    int makeOfflineEntry(const std::string& sender, const std::string& recipient) {
        UserRepository userRepo;
        for (const auto& name : {sender, recipient}) {
            if (!userRepo.findByUsername(name)) userRepo.save(User{0, name, "hash:x", ""});
        }

        MessageRepository msgRepo;
        Message msg{0, sender, recipient, "ciphertext==", "wrappedkey==", "sent"};
        msgRepo.save(msg);
        auto msgs = msgRepo.findByRecipient(recipient);
        int msgId = msgs.back().getId();

        OfflineQueueRepository offlineRepo;
        offlineRepo.save(OfflineMessage{0, msgId, recipient, ""});
        auto pending = offlineRepo.findUndeliveredByRecipient(recipient);
        return pending.back().getId();
    }

    void removeFixtures(const std::string& sender, const std::string& recipient) {
        OfflineQueueRepository offlineRepo;
        MessageRepository msgRepo;
        UserRepository userRepo;
        for (const auto& m : msgRepo.findByRecipient(recipient)) msgRepo.remove(m.getId());
        for (const auto& name : {sender, recipient}) {
            auto u = userRepo.findByUsername(name);
            if (u) userRepo.remove(u->getId());
        }
    }

private slots:
    void initTestCase() {
        const char* url = std::getenv("DATABASE_URL");
        if (!url) return;
        try {
            DbManager::instance().init(std::string(url));
            _dbAvailable = true;
        } catch (...) {
        }
    }

    // UT-BE-12a: incrementAttempts persists and entry stays retryable below the cap
    void incrementAttemptsPersists() {
        if (!_dbAvailable) QSKIP("DATABASE_URL not set");

        const std::string sender = "ut_be_12a_sender";
        const std::string recipient = "ut_be_12a_recipient";
        OfflineQueueRepository offlineRepo;

        int entryId = makeOfflineEntry(sender, recipient);
        offlineRepo.incrementAttempts(entryId);
        offlineRepo.incrementAttempts(entryId);

        auto entry = offlineRepo.findById(entryId);
        QVERIFY(entry.has_value());
        QCOMPARE(entry->getDeliveryAttempts(), 2);
        QVERIFY(!entry->isDelivered());

        // still below MAX_DELIVERY_ATTEMPTS -> still returned for delivery
        auto pending = offlineRepo.findUndeliveredByRecipient(recipient);
        QVERIFY(!pending.empty());
        QCOMPARE(pending.back().getId(), entryId);

        offlineRepo.remove(entryId);
        removeFixtures(sender, recipient);
    }

    // UT-BE-12b: entries at the cap are excluded from delivery and purged
    void exhaustedEntriesExcluded() {
        if (!_dbAvailable) QSKIP("DATABASE_URL not set");

        const std::string sender = "ut_be_12b_sender";
        const std::string recipient = "ut_be_12b_recipient";
        OfflineQueueRepository offlineRepo;

        int entryId = makeOfflineEntry(sender, recipient);
        for (int i = 0; i < OfflineQueueRepository::MAX_DELIVERY_ATTEMPTS; ++i)
            offlineRepo.incrementAttempts(entryId);

        // exhausted -> no longer offered for delivery
        auto pending = offlineRepo.findUndeliveredByRecipient(recipient);
        QVERIFY(pending.empty());

        // but the row still exists until cleanup purges it
        QVERIFY(offlineRepo.findById(entryId).has_value());
        offlineRepo.cleanupExhausted();
        QVERIFY(!offlineRepo.findById(entryId).has_value());

        removeFixtures(sender, recipient);
    }
};

QTEST_MAIN(OfflineQueueRepositoryTest)
#include "OfflineQueueRepositoryTest.moc"
