#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles ACK packets. L3 under BaseAuthHandler.
 *
 * Two forms:
 * - Legacy 1:1: packet.body holds a message id as a decimal string; the
 *   message is marked delivered and removed from offline_queue.
 * - Group delivered receipt (errorMsg == "delivered"): packet.to holds the
 *   numeric group id, packet.body the message timestamp. The member's
 *   receipt row is marked delivered and the event is forwarded to the
 *   message sender (offline-queued if the sender is away).
 */
class AckHandler : public BaseAuthHandler {
public:
    explicit AckHandler(Server& server) : _server(server) {}

protected:
    void validate(const Packet& packet) override;
    void execute(Packet& packet, ClientSession& session) override;

private:
    /** @brief Group delivered receipt: mark row, forward to sender or queue. */
    void executeGroupDelivered(Packet& packet, ClientSession& session);

    Server& _server;
};
