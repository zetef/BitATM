#pragma once
#include "BaseAuthHandler.h"

class ClientSession;

/**
 * @brief Handles LOGIN packets. L3 under BaseAuthHandler.
 *
 * Overrides authorize() to require a non-authenticated session
 * (rejects double-login). Overrides execute() to verify credentials,
 * issue a CSPRNG 256-bit session token, flush offline messages, and
 * flush queued read receipts.
 */
class LoginHandler : public BaseAuthHandler {
protected:
    void validate(const Packet& packet) override;
    void authorize(const ClientSession& session) override;
    void execute(Packet& packet, ClientSession& session) override;

private:
    /** @brief Deliver any read receipts queued while username was offline. */
    void flushOfflineReadReceipts(const std::string& username, ClientSession& session);
};
