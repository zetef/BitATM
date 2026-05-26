#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles DELETE_GROUP packets. L3 under BaseAuthHandler.
 *
 * Only the group creator may delete the group. On deletion all online
 * members receive a GROUP_LEAVE notification and the group row (along
 * with cascade-deleted group_members, group_keys and group_messages) is
 * removed from the database.
 */
class DeleteGroupHandler : public BaseAuthHandler {
public:
    /** @brief Construct with a reference to the server for client lookup. */
    explicit DeleteGroupHandler(Server& server) : _server(server) {}

protected:
    /**
     * @brief Validate that to (group_id) is present.
     * @throws ProtocolException if the group_id field is missing.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Verify creator role, fan GROUP_LEAVE to all online members, then
     *        delete the group row from the database.
     */
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
