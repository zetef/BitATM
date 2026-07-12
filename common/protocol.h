#pragma once
#include <cstdint>
#include <iostream>
#include <string>

#include "AppException.h"

constexpr int PROTOCOL_VERSION = 1;
constexpr int MAX_PACKET_SIZE = 65536;

// WebSocket keepalive contract: the client sends a protocol-level ping every
// WS_CLIENT_PING_INTERVAL_SEC; the server closes sockets silent for longer
// than WS_RECEIVE_TIMEOUT_SEC. The interval must stay well under the timeout.
constexpr int WS_CLIENT_PING_INTERVAL_SEC = 30;
constexpr int WS_RECEIVE_TIMEOUT_SEC = 600;

enum class PacketType {
    LOGIN,
    REGISTER,
    MESSAGE,
    KEY_EXCHANGE,
    ACK,
    ERR,
    SYNC_HISTORY,
    CREATE_GROUP,
    READ_RECEIPT,
    GROUP_MESSAGE,
    GROUP_KEY_EXCHANGE,
    GROUP_INVITE,
    GROUP_LEAVE,
    GROUP_INFO,
    DELETE_GROUP
};

struct Packet {
    PacketType type;
    int version = PROTOCOL_VERSION;
    std::string from;
    std::string to;
    std::string body;  // ciphertext              (base64)
    std::string key;   // RSA-encrypted AES key   (base64)
    std::string timestamp;
    std::string errorMsg;

    Packet() = default;
    Packet(const Packet&) = default;
    Packet(Packet&&) noexcept = default;
    Packet& operator=(const Packet&) = default;
    Packet& operator=(Packet&&) noexcept = default;
    ~Packet() = default;

    bool operator==(const Packet& other) const {
        return type == other.type && version == other.version && from == other.from &&
               to == other.to && body == other.body && key == other.key &&
               timestamp == other.timestamp && errorMsg == other.errorMsg;
    }

    friend std::ostream& operator<<(std::ostream& os, const Packet& p) {
        os << "Packet{type=" << static_cast<int>(p.type) << ", v=" << p.version
           << ", from=" << p.from << ", to=" << p.to << ", body=" << p.body
           << ", ts=" << p.timestamp;

        if (!p.errorMsg.empty()) os << ", err=" << p.errorMsg;
        os << "}";
        return os;
    }

    // pipe delimited: type|version|from|to|body|key|timestamp|errorMsg
    friend std::istream& operator>>(std::istream& is, Packet& p) {
        int t;
        char sep;
        is >> t >> sep >> p.version >> sep;
        std::getline(is, p.from, '|');
        std::getline(is, p.to, '|');
        std::getline(is, p.body, '|');
        std::getline(is, p.key, '|');
        std::getline(is, p.timestamp, '|');
        std::getline(is, p.errorMsg);
        if (is.eof()) is.clear();  // EOF on last field is expected - errorMsg may be empty
        if (t < 0 || t > static_cast<int>(PacketType::DELETE_GROUP))
            throw ProtocolException("unknown packet type: " + std::to_string(t));
        p.type = static_cast<PacketType>(t);
        return is;
    }
};
