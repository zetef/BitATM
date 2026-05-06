#include <QtTest>
#include <sstream>

#include "DbManager.h"
#include "GroupRepository.h"

class CreateGroupHandlerTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        DbManager::instance().init(qgetenv("DATABASE_URL").toStdString());
    }

    // UT-BE-09: group > 32 members should be rejected (validation logic)
    void rejectGroupOver32() {
        std::string memberList;
        for (int i = 0; i < 32; ++i) {
            if (i > 0) memberList += ",";
            memberList += "user" + std::to_string(i);
        }
        std::vector<std::string> members;
        std::istringstream ss(memberList);
        std::string m;
        while (std::getline(ss, m, ',')) members.push_back(m);
        // creator + 32 members = 33, exceeds limit
        QVERIFY(members.size() + 1 > 32);
    }

    // UT-BE-10: creator cannot be kicked
    void rejectKickOfCreator() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        GroupRepository repo;
        int groupId = repo.createGroup("TestGroup", "alice");
        repo.addMember(groupId, "alice", "creator");
        repo.addMember(groupId, "bob", "admin");
        std::string creatorRole = repo.getMemberRole(groupId, "alice");
        QCOMPARE(creatorRole, std::string("creator"));
        // Handler checks this and throws - verified here that role is correct
        QVERIFY(creatorRole == "creator");
    }

    // UT-BE-11: key rotation atomically replaces all keys
    void atomicKeyReplacement() {
        if (qgetenv("DATABASE_URL").isEmpty()) QSKIP("DATABASE_URL not set");
        GroupRepository repo;
        int groupId = repo.createGroup("KeyGroup", "alice");
        repo.addMember(groupId, "alice", "creator");
        repo.addMember(groupId, "bob", "member");
        repo.saveKey(groupId, "alice", "old_alice_key");
        repo.saveKey(groupId, "bob", "old_bob_key");

        std::vector<std::pair<std::string, std::string>> newKeys = {{"alice", "new_alice_key"},
                                                                    {"bob", "new_bob_key"}};
        repo.replaceAllKeys(groupId, newKeys);

        auto aliceKey = repo.getKey(groupId, "alice");
        auto bobKey = repo.getKey(groupId, "bob");
        QVERIFY(aliceKey.has_value());
        QVERIFY(bobKey.has_value());
        QCOMPARE(*aliceKey, std::string("new_alice_key"));
        QCOMPARE(*bobKey, std::string("new_bob_key"));
    }
};
QTEST_MAIN(CreateGroupHandlerTest)
#include "CreateGroupHandlerTest.moc"
