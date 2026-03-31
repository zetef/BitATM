#pragma once
#include <string>

#include "../../common/protocol.h"

/**
 * @brief Stateless utility for serializing and deserializing Packets.
 *
 * Wraps the Packet wire format (pipe-delimited) behind a clean API.
 * Used by UT-BE-03 (deserialize all 6 types) and UT-BE-04 (round-trip).
 * Throws ProtocolException on malformed input.
 */
class ProtocolParser {
public:
    ProtocolParser() = default;
    ~ProtocolParser() = default;

    // Non-copyable - stateless utility, no reason to copy.
    ProtocolParser(const ProtocolParser&) = delete;
    ProtocolParser& operator=(const ProtocolParser&) = delete;
    ProtocolParser(ProtocolParser&&) = delete;
    ProtocolParser& operator=(ProtocolParser&&) = delete;

    /** @brief Serialize a Packet to its pipe-delimited wire representation. */
    std::string serialize(const Packet& packet) const;

    /** @brief Deserialize a pipe-delimited string into a Packet.
     *  @throws ProtocolException if the string is malformed or truncated. */
    Packet deserialize(const std::string& data) const;
};
