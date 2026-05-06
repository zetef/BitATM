#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles GROUP_KEY_EXCHANGE packets. L3 under BaseAuthHandler.
 *
 * Only a group creator or admin may issue a key exchange.
 * A single-entry key field adds one member's key (member add flow).
 * A multi-entry key field atomically rotates all keys (kick flow).
 */
class GroupKeyExchangeHandler : public BaseAuthHandler {
public:
    /** @brief Construct with a reference to the server for client lookup. */
    explicit GroupKeyExchangeHandler(Server& server) : _server(server) {}

protected:
    /**
     * @brief Validate that to (group_id) and key are present.
     * @throws ProtocolException if any required field is missing.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Store keys (single add or full rotation) and notify affected members.
     */
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
