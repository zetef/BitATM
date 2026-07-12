#include "ClientSession.h"

#include <Poco/Net/WebSocket.h>

#include <sstream>

#include "../../common/AppException.h"
#include "ProtocolParser.h"

ClientSession::ClientSession(Poco::Net::WebSocket ws) : _ws(std::move(ws)) {}

void ClientSession::send(const Packet& packet) {
    try {
        ProtocolParser parser;
        const std::string frame = parser.serialize(packet);
        _ws.sendFrame(frame.data(), static_cast<int>(frame.size()),
                      Poco::Net::WebSocket::FRAME_TEXT);
    } catch (const Poco::Exception& e) {
        throw NetworkException("ClientSession::send failed: " + e.message());
    }
}

bool ClientSession::receive(Packet& packet) {
    try {
        char buf[MAX_PACKET_SIZE];
        while (true) {
            int flags = 0;
            int n = _ws.receiveFrame(buf, sizeof(buf), flags);

            const int op = flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;
            if (op == Poco::Net::WebSocket::FRAME_OP_CLOSE) {
                setState(State::Disconnected);
                return false;
            }
            // Poco does not auto-reply to pings; answer here so keepalive
            // frames never reach the protocol parser.
            if (op == Poco::Net::WebSocket::FRAME_OP_PING) {
                _ws.sendFrame(buf, n,
                              static_cast<int>(Poco::Net::WebSocket::FRAME_FLAG_FIN) |
                                  static_cast<int>(Poco::Net::WebSocket::FRAME_OP_PONG));
                continue;
            }
            if (op == Poco::Net::WebSocket::FRAME_OP_PONG) {
                continue;
            }
            if (n <= 0) {
                setState(State::Disconnected);
                return false;
            }

            ProtocolParser parser;
            packet = parser.deserialize(std::string(buf, static_cast<std::size_t>(n)));
            return true;
        }
    } catch (const Poco::Exception& e) {
        throw NetworkException("ClientSession::receive failed: " + e.message());
    }
}

ClientSession::State ClientSession::getState() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _state;
}

void ClientSession::setState(State s) {
    std::lock_guard<std::mutex> lock(_mutex);
    _state = s;
}

const std::string& ClientSession::getUsername() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _username;
}

void ClientSession::setUsername(const std::string& u) {
    std::lock_guard<std::mutex> lock(_mutex);
    _username = u;
}

const std::string& ClientSession::getSessionToken() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _sessionToken;
}

void ClientSession::setSessionToken(const std::string& t) {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessionToken = t;
}

bool ClientSession::isAuthenticated() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _state == State::Authenticated;
}
