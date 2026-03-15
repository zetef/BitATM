#pragma once
#include <cstdint>
#include <iostream>
#include <string>

constexpr int PROTOCOL_VERSION = 1;
constexpr int MAX_PACKET_SIZE = 65536;

enum class PacketType { LOGIN, REGISTER, MESSAGE, KEY_EXCHANGE, ACK, ERROR };

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

    bool operator==(const Packet& other) const;
    friend std::ostream& operator<<(std::ostream& os, const Packet& p);
    friend std::istream& operator>>(std::istream& is, Packet& p);
};