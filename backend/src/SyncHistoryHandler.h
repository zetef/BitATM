#pragma once
#include "BaseAuthHandler.h"

/**
 * @brief Handles SYNC_HISTORY packets. L3 under BaseAuthHandler.
 *
 * Fetches all stored messages for the authenticated user and sends
 * them back in chronological order, followed by an ACK.
 */
class SyncHistoryHandler : public BaseAuthHandler {
protected:
    void validate(const Packet& packet) override;
    void execute(Packet& packet, ClientSession& session) override;
};
