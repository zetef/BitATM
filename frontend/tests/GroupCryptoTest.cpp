#include <QtTest>

#include "../../common/protocol.h"
#include "CryptoEngine.h"

class GroupCryptoTest : public QObject {
    Q_OBJECT
private slots:
    // UT-FE-14: CREATE_GROUP packet has correct type and fields
    void createGroupPacketFields() {
        Packet p;
        p.type = PacketType::CREATE_GROUP;
        p.from = "alice";
        p.errorMsg = "MyGroup";
        p.body = "bob,carol";
        p.key = "bob:enckey1|carol:enckey2|alice:enckey3";

        QCOMPARE(p.type, PacketType::CREATE_GROUP);
        QCOMPARE(p.from, std::string("alice"));
        QCOMPARE(p.errorMsg, std::string("MyGroup"));
        QVERIFY(!p.key.empty());
    }

    // UT-FE-15: group message AES roundtrip via CryptoEngine
    void groupMessageRoundtrip() {
        CryptoEngine crypto;
        QByteArray aesKey = crypto.generateAESKey();
        QString plaintext = "Hello, group!";
        QString ciphertext = crypto.encrypt(plaintext, aesKey);
        QString decrypted = crypto.decrypt(ciphertext, aesKey);

        QCOMPARE(decrypted, plaintext);
        QVERIFY(ciphertext != plaintext);
    }
};
QTEST_MAIN(GroupCryptoTest)
#include "GroupCryptoTest.moc"
