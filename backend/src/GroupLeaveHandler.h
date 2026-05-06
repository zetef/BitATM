#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles GROUP_LEAVE packets. L3 under BaseAuthHandler.
 *
 * Supports two sub-commands:
 * - Voluntary leave: body is empty, the authenticated user leaves.
 * - Admin kick: body contains the target username (actor must be creator/admin;
 *   admins cannot kick other admins unless the actor is the creator).
 * The creator can neither leave nor be kicked.
 * After a kick the actor receives a rotation-needed signal.
 */
class GroupLeaveHandler : public BaseAuthHandler {
public:
    /** @brief Construct with a reference to the server for client lookup. */
    explicit GroupLeaveHandler(Server& server) : _server(server) {}

protected:
    /**
     * @brief Validate that to (group_id) is present.
     * @throws ProtocolException if the group_id field is missing.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Process leave or kick, notify target if online, signal rotation if kick.
     */
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
