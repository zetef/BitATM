#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles CREATE_GROUP packets. L3 under BaseAuthHandler.
 *
 * Validates group name, member list (2-32 total), and key map.
 * Creates the group, inserts all members and their encrypted AES keys,
 * fans GROUP_INVITE to online members, and sends GROUP_INFO back to creator.
 */
class CreateGroupHandler : public BaseAuthHandler {
public:
    /** @brief Construct with a reference to the server for client lookup. */
    explicit CreateGroupHandler(Server& server) : _server(server) {}

protected:
    /**
     * @brief Validate that group name, member list, and key map are present and within limits.
     * @throws ProtocolException if any field is invalid.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Create the group, store keys, fan out invites, reply with GROUP_INFO.
     */
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
