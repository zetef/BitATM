#pragma once
#include "BaseAuthHandler.h"

/**
 * @brief Handles LOGIN packets. L3 under BaseAuthHandler.
 *
 * Overrides authorize() to require a non-authenticated session
 * (rejects double-login). Overrides execute() to verify credentials,
 * issue a CSPRNG 256-bit session token, and flush offline messages.
 */
class LoginHandler : public BaseAuthHandler {
protected:
    void validate(const Packet& packet) override;
    void authorize(const ClientSession& session) override;
    void execute(Packet& packet, ClientSession& session) override;
};
