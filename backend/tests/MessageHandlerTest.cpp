#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/WebSocket.h>

#include <QtTest>
#include <cstdlib>

#include "ProtocolParser.h"

static Packet sendRecv(Poco::Net::WebSocket& ws, const Packet& out) {
    ProtocolParser parser;
    std::string data = parser.serialize(out);
    ws.sendFrame(data.data(), static_cast<int>(data.size()), Poco::Net::WebSocket::FRAME_TEXT);
    char buf[65536];
    int flags = 0;
    int n = ws.receiveFrame(buf, sizeof(buf), flags);
    return parser.deserialize(std::string(buf, n));
}

static Poco::Net::WebSocket* openWs(const std::string& host, int port,
                                    Poco::Net::HTTPClientSession& cs) {
    Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, "/",
                               Poco::Net::HTTPMessage::HTTP_1_1);
    req.set("Host", host + ":" + std::to_string(port));
    Poco::Net::HTTPResponse res;
    return new Poco::Net::WebSocket(cs, req, res);
}

static bool serverReachable(const std::string& host, int port) {
    try {
        Poco::Net::HTTPClientSession cs(host, port);
        Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, "/",
                                   Poco::Net::HTTPMessage::HTTP_1_1);
        req.set("Host", host + ":" + std::to_string(port));
        Poco::Net::HTTPResponse res;
        Poco::Net::WebSocket ws(cs, req, res);
        ws.close();
        return true;
    } catch (...) {
        return false;
    }
}

class MessageHandlerTest : public QObject {
    Q_OBJECT
private:
    std::string _host = "localhost";
    int _port = 8080;

    void initTestCase() {
        const char* h = std::getenv("BITATM_TEST_HOST");
        if (h) _host = h;
        const char* p = std::getenv("BITATM_TEST_PORT");
        if (p) _port = std::atoi(p);
    }

private slots:
    // UT-BE-06a: message to offline user -> server stores it and ACKs sender
    void messageToOfflineUserIsQueued() {
        if (!serverReachable(_host, _port))
            QSKIP("Test server not reachable (start it locally or set BITATM_TEST_HOST/PORT)");

        // login as sender (register first if needed)
        Poco::Net::HTTPClientSession cs1(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> ws1(openWs(_host, _port, cs1));

        Packet reg;
        reg.type = PacketType::REGISTER;
        reg.from = "ut_be_06_sender";
        reg.body = "SenderPass123!";
        sendRecv(*ws1, reg);  // ignore - may already exist

        Packet login;
        login.type = PacketType::LOGIN;
        login.from = "ut_be_06_sender";
        login.body = "SenderPass123!";
        Packet loginResp = sendRecv(*ws1, login);
        QCOMPARE(static_cast<int>(loginResp.type), static_cast<int>(PacketType::ACK));

        // send message to a recipient who is definitely offline
        Packet msg;
        msg.type = PacketType::MESSAGE;
        msg.from = "ut_be_06_sender";
        msg.to = "ut_be_06_offline_recipient";
        msg.body = "encrypted_body_base64==";
        msg.key = "wrapped_aes_key_base64==";
        Packet ack = sendRecv(*ws1, msg);

        // server should ACK the sender even when recipient is offline
        QCOMPARE(static_cast<int>(ack.type), static_cast<int>(PacketType::ACK));

        ws1->close();
    }

    // UT-BE-06b: message to online user -> server forwards it and ACKs sender
    void messageToOnlineUserIsForwarded() {
        if (!serverReachable(_host, _port)) QSKIP("Test server not reachable");

        // connect and login as sender
        Poco::Net::HTTPClientSession cs1(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> ws1(openWs(_host, _port, cs1));

        Packet reg1;
        reg1.type = PacketType::REGISTER;
        reg1.from = "ut_be_06_sender";
        reg1.body = "SenderPass123!";
        sendRecv(*ws1, reg1);

        Packet login1;
        login1.type = PacketType::LOGIN;
        login1.from = "ut_be_06_sender";
        login1.body = "SenderPass123!";
        sendRecv(*ws1, login1);

        // connect and login as recipient
        Poco::Net::HTTPClientSession cs2(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> ws2(openWs(_host, _port, cs2));

        Packet reg2;
        reg2.type = PacketType::REGISTER;
        reg2.from = "ut_be_06_recipient";
        reg2.body = "RecipientPass123!";
        sendRecv(*ws2, reg2);

        Packet login2;
        login2.type = PacketType::LOGIN;
        login2.from = "ut_be_06_recipient";
        login2.body = "RecipientPass123!";
        sendRecv(*ws2, login2);

        // sender sends message to online recipient
        Packet msg;
        msg.type = PacketType::MESSAGE;
        msg.from = "ut_be_06_sender";
        msg.to = "ut_be_06_recipient";
        msg.body = "encrypted_body_base64==";
        msg.key = "wrapped_aes_key_base64==";
        Packet ack = sendRecv(*ws1, msg);

        QCOMPARE(static_cast<int>(ack.type), static_cast<int>(PacketType::ACK));

        // recipient's connection should receive the forwarded message
        char buf[65536];
        int flags = 0;
        ws2->setTimeout(Poco::Timespan(2, 0));  // 2s timeout
        int n = 0;
        try {
            n = ws2->receiveFrame(buf, sizeof(buf), flags);
        } catch (...) {
            QFAIL("Recipient did not receive the forwarded message within 2s");
        }
        ProtocolParser parser;
        Packet forwarded = parser.deserialize(std::string(buf, n));
        QCOMPARE(static_cast<int>(forwarded.type), static_cast<int>(PacketType::MESSAGE));
        QVERIFY(forwarded.from == "ut_be_06_sender");

        ws1->close();
        ws2->close();
    }
};

QTEST_MAIN(MessageHandlerTest)
#include "MessageHandlerTest.moc"
