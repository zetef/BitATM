#pragma once
#include "BaseAuthHandler.h"

/**
 * @brief Handles ACK packets. L3 under BaseAuthHandler.
 *
 * Client sends ACK after successfully receiving and decrypting a message.
 * Server marks the message as delivered and removes it from offline_queue.
 * packet.body holds the message id as a decimal string.
 */
class AckHandler : public BaseAuthHandler {
protected:
    void validate(const Packet& packet) override;
    void execute(Packet& packet, ClientSession& session) override;
};
