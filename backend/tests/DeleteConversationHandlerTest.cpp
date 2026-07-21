#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/WebSocket.h>

#include <QtTest>
#include <cstdlib>
#include <memory>

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

static void registerAndLogin(Poco::Net::WebSocket& ws, const std::string& user,
                             const std::string& pass) {
    Packet reg;
    reg.type = PacketType::REGISTER;
    reg.from = user;
    reg.body = pass;
    sendRecv(ws, reg);  // ignore result - may already exist

    Packet login;
    login.type = PacketType::LOGIN;
    login.from = user;
    login.body = pass;
    Packet loginResp = sendRecv(ws, login);
    QCOMPARE(static_cast<int>(loginResp.type), static_cast<int>(PacketType::ACK));
}

class DeleteConversationHandlerTest : public QObject {
    Q_OBJECT
private:
    std::string _host = "localhost";
    int _port = 8080;

private slots:
    void initTestCase() {
        const char* h = std::getenv("BITATM_TEST_HOST");
        if (h) _host = h;
        const char* p = std::getenv("BITATM_TEST_PORT");
        if (p) _port = std::atoi(p);
    }

    // peer online -> DELETE_CONVERSATION forwarded immediately
    void peerOnlineReceivesDeleteImmediately() {
        if (!serverReachable(_host, _port)) QSKIP("Test server not reachable");

        Poco::Net::HTTPClientSession cs1(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> actorWs(openWs(_host, _port, cs1));
        registerAndLogin(*actorWs, "ut_delconv_actor", "ActorPass123!");

        Poco::Net::HTTPClientSession cs2(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> peerWs(openWs(_host, _port, cs2));
        registerAndLogin(*peerWs, "ut_delconv_peer_online", "PeerPass123!");

        Packet del;
        del.type = PacketType::DELETE_CONVERSATION;
        del.from = "ut_delconv_actor";
        del.to = "ut_delconv_peer_online";
        sendRecv(*actorWs, del);

        peerWs->setReceiveTimeout(Poco::Timespan(2, 0));
        char buf[65536];
        int flags = 0;
        int n = 0;
        try {
            n = peerWs->receiveFrame(buf, sizeof(buf), flags);
        } catch (...) {
            QFAIL("Peer did not receive DELETE_CONVERSATION within 2s");
        }
        ProtocolParser parser;
        Packet received = parser.deserialize(std::string(buf, n));
        QCOMPARE(static_cast<int>(received.type),
                 static_cast<int>(PacketType::DELETE_CONVERSATION));
        QCOMPARE(received.from, std::string("ut_delconv_actor"));

        actorWs->close();
        peerWs->close();
    }

    // peer offline at delete time -> receives DELETE_CONVERSATION on next login
    void peerOfflineReceivesDeleteOnNextLogin() {
        if (!serverReachable(_host, _port)) QSKIP("Test server not reachable");

        Poco::Net::HTTPClientSession cs1(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> actorWs(openWs(_host, _port, cs1));
        registerAndLogin(*actorWs, "ut_delconv_actor2", "ActorPass123!");

        // register the peer but never connect them - simulates "offline"
        Poco::Net::HTTPClientSession csReg(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> regWs(openWs(_host, _port, csReg));
        Packet reg;
        reg.type = PacketType::REGISTER;
        reg.from = "ut_delconv_peer_offline";
        reg.body = "PeerPass123!";
        sendRecv(*regWs, reg);
        regWs->close();

        Packet del;
        del.type = PacketType::DELETE_CONVERSATION;
        del.from = "ut_delconv_actor2";
        del.to = "ut_delconv_peer_offline";
        Packet ack = sendRecv(*actorWs, del);
        QVERIFY(static_cast<int>(ack.type) != static_cast<int>(PacketType::ERR));
        actorWs->close();

        // now the peer logs in and should receive the queued DELETE_CONVERSATION
        Poco::Net::HTTPClientSession cs2(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> peerWs(openWs(_host, _port, cs2));
        Packet login;
        login.type = PacketType::LOGIN;
        login.from = "ut_delconv_peer_offline";
        login.body = "PeerPass123!";
        Packet loginResp = sendRecv(*peerWs, login);
        QCOMPARE(static_cast<int>(loginResp.type), static_cast<int>(PacketType::ACK));

        peerWs->setReceiveTimeout(Poco::Timespan(2, 0));
        bool sawDelete = false;
        for (int i = 0; i < 5 && !sawDelete; ++i) {
            char buf[65536];
            int flags = 0;
            int n;
            try {
                n = peerWs->receiveFrame(buf, sizeof(buf), flags);
            } catch (...) {
                break;
            }
            ProtocolParser parser;
            Packet p = parser.deserialize(std::string(buf, n));
            if (p.type == PacketType::DELETE_CONVERSATION && p.from == "ut_delconv_actor2")
                sawDelete = true;
        }
        QVERIFY(sawDelete);

        peerWs->close();
    }
};
QTEST_MAIN(DeleteConversationHandlerTest)
#include "DeleteConversationHandlerTest.moc"
