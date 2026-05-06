#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles GROUP_MESSAGE packets. L3 under BaseAuthHandler.
 *
 * Verifies group membership, persists the ciphertext, fans it out to all
 * online members (excluding the sender), and sends an ACK back to the sender.
 */
class GroupMessageHandler : public BaseAuthHandler {
public:
    /** @brief Construct with a reference to the server for client lookup. */
    explicit GroupMessageHandler(Server& server) : _server(server) {}

protected:
    /**
     * @brief Validate that from, to (group_id), and body are present.
     * @throws ProtocolException if any required field is missing.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Persist and fan out the group message, then ACK the sender.
     */
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
