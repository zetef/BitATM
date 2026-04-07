#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles MESSAGE packets. L3 under BaseAuthHandler.
 *
 * The server is relay-only - body and key are opaque ciphertext.
 * If the recipient is online: forward directly via their session.
 * If offline: persist to messages + offline_queue for later delivery.
 */
class MessageHandler : public BaseAuthHandler {
public:
    explicit MessageHandler(Server& server) : _server(server) {}

protected:
    void validate(const Packet& packet) override;
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
