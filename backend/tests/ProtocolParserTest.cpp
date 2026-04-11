#include <QtTest>

#include "ProtocolParser.h"

class ProtocolParserTest : public QObject {
    Q_OBJECT
private slots:
    // UT-BE-03: all 6 packet types deserialize correctly
    void deserializeAllPacketTypes() {
        ProtocolParser parser;
        struct Case {
            PacketType type;
            std::string line;
        };
        Case cases[] = {
            {PacketType::LOGIN, "0|1|alice|bob|body||ts|"},
            {PacketType::REGISTER, "1|1|alice|bob|body||ts|"},
            {PacketType::MESSAGE, "2|1|alice|bob|body||ts|"},
            {PacketType::KEY_EXCHANGE, "3|1|alice|bob|body||ts|"},
            {PacketType::ACK, "4|1|alice|bob|body||ts|"},
            {PacketType::ERR, "5|1|alice|bob|body||ts|"},
        };
        for (const auto& c : cases) {
            Packet p = parser.deserialize(c.line);
            QCOMPARE(static_cast<int>(p.type), static_cast<int>(c.type));
            QVERIFY(p.from == "alice");
            QVERIFY(p.to == "bob");
            QVERIFY(p.body == "body");
            QCOMPARE(p.version, 1);
        }
    }

    // UT-BE-04: serialize -> deserialize round-trip preserves all fields
    void serializeRoundtrip() {
        ProtocolParser parser;
        Packet original;
        original.type = PacketType::MESSAGE;
        original.from = "alice";
        original.to = "bob";
        original.body = "ciphertext==";
        original.key = "wrappedkey==";
        original.timestamp = "2026-04-11T10:00:00Z";
        original.errorMsg = "";

        std::string wire = parser.serialize(original);
        Packet roundtripped = parser.deserialize(wire);

        QVERIFY(roundtripped == original);
    }
};

QTEST_MAIN(ProtocolParserTest)
#include "ProtocolParserTest.moc"
