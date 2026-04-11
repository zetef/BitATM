#include <QtTest>
#include <cstdlib>

#include "DbManager.h"
#include "Message.h"
#include "MessageRepository.h"
#include "OfflineMessage.h"
#include "OfflineQueueRepository.h"
#include "User.h"
#include "UserRepository.h"

class DbManagerTest : public QObject {
    Q_OBJECT
private:
    bool _dbAvailable = false;

    void initTestCase() {
        const char* url = std::getenv("DATABASE_URL");
        if (!url) return;
        try {
            DbManager::instance().init(std::string(url));
            _dbAvailable = true;
        } catch (...) {
        }
    }

private slots:
    // UT-BE-01: insert user -> fetch by username returns correct struct
    void insertUser() {
        if (!_dbAvailable) QSKIP("DATABASE_URL not set");

        UserRepository repo;
        const std::string name = "ut_be_01_user";

        // clean up from previous run if needed
        auto existing = repo.findByUsername(name);
        if (existing) repo.remove(existing->getId());

        User u{0, name, "hash:deadbeef", "pubkey_base64"};
        repo.save(u);

        auto found = repo.findByUsername(name);
        QVERIFY(found.has_value());
        QVERIFY(found->getUsername() == name);
        QVERIFY(found->getPasswordHash() == "hash:deadbeef");
        QVERIFY(found->getPublicKey() == "pubkey_base64");

        repo.remove(found->getId());
    }

    // UT-BE-02: store offline message -> retrieved correctly for offline user
    void storeOfflineMessage() {
        if (!_dbAvailable) QSKIP("DATABASE_URL not set");

        UserRepository userRepo;
        MessageRepository msgRepo;
        OfflineQueueRepository offlineRepo;

        const std::string sender = "ut_be_02_sender";
        const std::string recipient = "ut_be_02_recipient";

        // ensure users exist
        for (const auto& name : {sender, recipient}) {
            if (!userRepo.findByUsername(name)) userRepo.save(User{0, name, "hash:x", ""});
        }

        // insert message
        Message msg{0, sender, recipient, "ciphertext==", "wrappedkey==", "sent"};
        msgRepo.save(msg);

        // find the saved message id
        auto msgs = msgRepo.findByRecipient(recipient);
        QVERIFY(!msgs.empty());
        int msgId = msgs.back().getId();

        // queue it as offline
        OfflineMessage entry{0, msgId, recipient, ""};
        offlineRepo.save(entry);

        // verify retrieval
        auto pending = offlineRepo.findUndeliveredByRecipient(recipient);
        QVERIFY(!pending.empty());
        QVERIFY(pending.back().getRecipient() == recipient);
        QVERIFY(!pending.back().isDelivered());

        // clean up
        offlineRepo.remove(pending.back().getId());
        msgRepo.remove(msgId);
        auto s = userRepo.findByUsername(sender);
        auto r = userRepo.findByUsername(recipient);
        if (s) userRepo.remove(s->getId());
        if (r) userRepo.remove(r->getId());
    }
};

QTEST_MAIN(DbManagerTest)
#include "DbManagerTest.moc"
