#pragma once
#include "ICommandHandler.h"

/**
 * @brief Handles REGISTER packets. L2 — extends ICommandHandler directly
 * (no session authentication required to create an account).
 *
 * Flow: validate username/password presence → check duplicate → hash
 * password (PBKDF2-SHA256, 100 000 iterations) → persist User → ACK.
 */
class RegisterHandler : public ICommandHandler {
protected:
    void validate(const Packet& packet) override;
    void authorize(const ClientSession& session) override;
    void execute(Packet& packet, ClientSession& session) override;
};
