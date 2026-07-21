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
    sendRecv(ws, reg);

    Packet login;
    login.type = PacketType::LOGIN;
    login.from = user;
    login.body = pass;
    Packet loginResp = sendRecv(ws, login);
    QCOMPARE(static_cast<int>(loginResp.type), static_cast<int>(PacketType::ACK));
}

class DeleteGroupHandlerTest : public QObject {
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

    void offlineMemberReceivesGroupLeaveOnNextLogin() {
        if (!serverReachable(_host, _port)) QSKIP("Test server not reachable");

        // creator logs in and creates a group with an offline member
        Poco::Net::HTTPClientSession cs1(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> creatorWs(openWs(_host, _port, cs1));
        registerAndLogin(*creatorWs, "ut_delgrp_creator", "CreatorPass123!");

        // register the member but never connect them - simulates "offline"
        Poco::Net::HTTPClientSession csReg(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> regWs(openWs(_host, _port, csReg));
        Packet reg;
        reg.type = PacketType::REGISTER;
        reg.from = "ut_delgrp_offline_member";
        reg.body = "MemberPass123!";
        sendRecv(*regWs, reg);
        regWs->close();

        Packet create;
        create.type = PacketType::CREATE_GROUP;
        create.from = "ut_delgrp_creator";
        create.errorMsg = "TestGroup";                       // group name (validated non-empty)
        create.body = "ut_delgrp_offline_member";            // comma-separated member list
        create.key = "ut_delgrp_offline_member:dummykey==";  // "user:encryptedKey;..." map
        // CreateGroupHandler replies with GROUP_INFO (body = new group id), not ACK
        Packet createInfo = sendRecv(*creatorWs, create);
        QCOMPARE(static_cast<int>(createInfo.type), static_cast<int>(PacketType::GROUP_INFO));
        const std::string groupId = createInfo.body;

        Packet del;
        del.type = PacketType::DELETE_GROUP;
        del.from = "ut_delgrp_creator";
        del.to = groupId;
        Packet delAck = sendRecv(*creatorWs, del);
        QVERIFY(static_cast<int>(delAck.type) != static_cast<int>(PacketType::ERR));
        creatorWs->close();

        // offline member logs in and should receive the queued GROUP_LEAVE
        Poco::Net::HTTPClientSession cs2(_host, _port);
        std::unique_ptr<Poco::Net::WebSocket> memberWs(openWs(_host, _port, cs2));
        Packet login;
        login.type = PacketType::LOGIN;
        login.from = "ut_delgrp_offline_member";
        login.body = "MemberPass123!";
        Packet loginResp = sendRecv(*memberWs, login);
        QCOMPARE(static_cast<int>(loginResp.type), static_cast<int>(PacketType::ACK));

        memberWs->setReceiveTimeout(Poco::Timespan(2, 0));
        bool sawLeave = false;
        for (int i = 0; i < 5 && !sawLeave; ++i) {
            char buf[65536];
            int flags = 0;
            int n;
            try {
                n = memberWs->receiveFrame(buf, sizeof(buf), flags);
            } catch (...) {
                break;
            }
            ProtocolParser parser;
            Packet p = parser.deserialize(std::string(buf, n));
            if (p.type == PacketType::GROUP_LEAVE && p.body == groupId) sawLeave = true;
        }
        QVERIFY(sawLeave);

        memberWs->close();
    }
};
QTEST_MAIN(DeleteGroupHandlerTest)
#include "DeleteGroupHandlerTest.moc"
