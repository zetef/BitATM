#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include <QtTest>

#include "DbManager.h"
#include "Message.h"
#include "MessageRepository.h"
#include "OfflineQueueRepository.h"
#include "UserRepository.h"

class MessageRepositoryTest : public QObject {
    Q_OBJECT
private:
    void ensureUser(const std::string& name) {
        UserRepository userRepo;
        if (!userRepo.findByUsername(name)) userRepo.save(User{0, name, "hash:x", ""});
    }

private slots:
    void initTestCase() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        DbManager::instance().init(qgetenv("DATABASE_URL").toStdString());
        ensureUser("ut_msgrepo_alice");
        ensureUser("ut_msgrepo_bob");
        ensureUser("ut_msgrepo_carol");
    }

    // deleteConversation removes both directions between the pair, leaves a
    // third user's messages untouched, and does not fail when an
    // offline_queue row references one of the deleted messages.
    void deleteConversationRemovesPairOnly() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        MessageRepository msgRepo;

        Message aliceToBob{0, "ut_msgrepo_alice", "ut_msgrepo_bob", "body1==", "key1==", "", "sent",
                           ""};
        msgRepo.save(aliceToBob);
        Message bobToAlice{0, "ut_msgrepo_bob", "ut_msgrepo_alice", "body2==", "key2==", "", "sent",
                           ""};
        msgRepo.save(bobToAlice);
        Message aliceToCarol{
            0, "ut_msgrepo_alice", "ut_msgrepo_carol", "body3==", "key3==", "", "sent", ""};
        msgRepo.save(aliceToCarol);

        auto savedForBob = msgRepo.findByRecipient("ut_msgrepo_bob");
        QVERIFY(!savedForBob.empty());
        OfflineQueueRepository offlineRepo;
        OfflineMessage queued{0, savedForBob.back().getId(), "ut_msgrepo_bob", ""};
        offlineRepo.save(queued);  // simulates bob being offline when alice's message arrived

        msgRepo.deleteConversation("ut_msgrepo_alice", "ut_msgrepo_bob");

        auto remaining = msgRepo.findAllForUser("ut_msgrepo_alice");
        bool anyWithBob = false;
        bool foundCarolMessage = false;
        for (const auto& m : remaining) {
            if (m.getSender() == "ut_msgrepo_bob" || m.getRecipient() == "ut_msgrepo_bob")
                anyWithBob = true;
            if (m.getRecipient() == "ut_msgrepo_carol") foundCarolMessage = true;
        }
        QVERIFY(!anyWithBob);
        QVERIFY(foundCarolMessage);

        auto stillQueued = offlineRepo.findUndeliveredByRecipient("ut_msgrepo_bob");
        for (const auto& q : stillQueued) QVERIFY(q.getMessageId() != savedForBob.back().getId());
    }
};
QTEST_MAIN(MessageRepositoryTest)
#include "MessageRepositoryTest.moc"
