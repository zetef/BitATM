#pragma once
#include "BaseAuthHandler.h"

/**
 * @brief Handles KEY_EXCHANGE packets. L3 under BaseAuthHandler.
 *
 * Two sub-operations in one packet type:
 *  - Upload own public key: packet.key contains the PEM public key.
 *  - Request peer's public key: packet.to is set, server replies with
 *    a KEY_EXCHANGE packet carrying the peer's public key in packet.key.
 */
class KeyExchangeHandler : public BaseAuthHandler {
protected:
    void validate(const Packet& packet) override;
    void execute(Packet& packet, ClientSession& session) override;
};
