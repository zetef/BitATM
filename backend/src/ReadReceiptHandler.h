#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles READ_RECEIPT packets. L3 under BaseAuthHandler.
 *
 * Relays a read receipt from the recipient to the original message sender.
 * packet.from = recipient username (who read the message)
 * packet.to   = original sender username
 * packet.body = ISO timestamp of the message that was read
 *
 * If the original sender is offline the receipt is stored in
 * offline_read_receipts and flushed to them on their next login.
 */
class ReadReceiptHandler : public BaseAuthHandler {
public:
    explicit ReadReceiptHandler(Server& server) : _server(server) {}

protected:
    void validate(const Packet& packet) override;
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
