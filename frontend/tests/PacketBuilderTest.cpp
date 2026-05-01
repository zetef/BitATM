#include <QtTest>
#include <stdexcept>

#include "PacketBuilder.h"

class PacketBuilderTest : public QObject {
    Q_OBJECT
private slots:
    void buildAllFields();     // UT-FE-07
    void missingTypeThrows();  // UT-FE-08
};

// UT-FE-07: all fields present, version == PROTOCOL_VERSION
void PacketBuilderTest::buildAllFields() {
    Packet p = PacketBuilder()
                   .setType(PacketType::MESSAGE)
                   .setFrom("alice")
                   .setTo("bob")
                   .setBody("encryptedBody")
                   .setKey("wrappedAesKey")
                   .build();

    QCOMPARE(p.type, PacketType::MESSAGE);
    QCOMPARE(p.version, PROTOCOL_VERSION);
    QCOMPARE(p.from, std::string("alice"));
    QCOMPARE(p.to, std::string("bob"));
    QCOMPARE(p.body, std::string("encryptedBody"));
    QCOMPARE(p.key, std::string("wrappedAesKey"));
    QVERIFY(!p.timestamp.empty());
}

// UT-FE-08: build() without setType() -> std::logic_error
void PacketBuilderTest::missingTypeThrows() {
    QVERIFY_THROWS_EXCEPTION(std::logic_error,
                             PacketBuilder().setFrom("alice").setTo("bob").build());
}

QTEST_MAIN(PacketBuilderTest)
#include "PacketBuilderTest.moc"
