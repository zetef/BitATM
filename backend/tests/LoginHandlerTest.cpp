#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/WebSocket.h>

#include <QtTest>
#include <cstdlib>

#include "ProtocolParser.h"

// Send one packet and return the server's next response.
static Packet sendRecv(Poco::Net::WebSocket& ws, const Packet& out) {
    ProtocolParser parser;
    std::string data = parser.serialize(out);
    ws.sendFrame(data.data(), static_cast<int>(data.size()), Poco::Net::WebSocket::FRAME_TEXT);

    char buf[65536];
    int flags = 0;
    int n = ws.receiveFrame(buf, sizeof(buf), flags);
    return parser.deserialize(std::string(buf, n));
}

// Returns true if a fresh WebSocket connection to the test server succeeds.
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

class LoginHandlerTest : public QObject {
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
    // UT-BE-05a: valid credentials -> server responds with ACK + session token
    void validCredentialsGiveAck() {
        if (!serverReachable(_host, _port))
            QSKIP("Test server not reachable (start it locally or set BITATM_TEST_HOST/PORT)");

        Poco::Net::HTTPClientSession cs(_host, _port);
        Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, "/",
                                   Poco::Net::HTTPMessage::HTTP_1_1);
        req.set("Host", _host + ":" + std::to_string(_port));
        Poco::Net::HTTPResponse res;
        Poco::Net::WebSocket ws(cs, req, res);

        // register test user (may already exist - ignore response)
        Packet reg;
        reg.type = PacketType::REGISTER;
        reg.from = "ut_be_05_user";
        reg.body = "TestPass123!";
        sendRecv(ws, reg);
        ws.close();

        // open a fresh connection and login
        Poco::Net::HTTPClientSession cs2(_host, _port);
        Poco::Net::HTTPRequest req2(Poco::Net::HTTPRequest::HTTP_GET, "/",
                                    Poco::Net::HTTPMessage::HTTP_1_1);
        req2.set("Host", _host + ":" + std::to_string(_port));
        Poco::Net::HTTPResponse res2;
        Poco::Net::WebSocket ws2(cs2, req2, res2);

        Packet login;
        login.type = PacketType::LOGIN;
        login.from = "ut_be_05_user";
        login.body = "TestPass123!";
        Packet resp = sendRecv(ws2, login);

        QCOMPARE(static_cast<int>(resp.type), static_cast<int>(PacketType::ACK));
        QVERIFY(!resp.body.empty());  // body carries the session token

        ws2.close();
    }

    // UT-BE-05b: wrong password -> server responds with ERR
    void badCredentialsGiveError() {
        if (!serverReachable(_host, _port)) QSKIP("Test server not reachable");

        Poco::Net::HTTPClientSession cs(_host, _port);
        Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, "/",
                                   Poco::Net::HTTPMessage::HTTP_1_1);
        req.set("Host", _host + ":" + std::to_string(_port));
        Poco::Net::HTTPResponse res;
        Poco::Net::WebSocket ws(cs, req, res);

        Packet login;
        login.type = PacketType::LOGIN;
        login.from = "ut_be_05_user";
        login.body = "WrongPassword999!";
        Packet resp = sendRecv(ws, login);

        QCOMPARE(static_cast<int>(resp.type), static_cast<int>(PacketType::ERR));

        ws.close();
    }
};

QTEST_MAIN(LoginHandlerTest)
#include "LoginHandlerTest.moc"
