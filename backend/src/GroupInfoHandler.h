#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles GROUP_INFO packets. L3 under BaseAuthHandler.
 *
 * Returns current group membership and roles to the requester.
 * Also handles the "grant_admin" and "revoke_admin" sub-commands (only the
 * group creator may grant/revoke admin status); on either, every other
 * online member is fanned a fresh GROUP_INFO too, so anyone with the group
 * info sheet open sees the role change live instead of only on next fetch.
 */
class GroupInfoHandler : public BaseAuthHandler {
public:
    /** @brief Construct with a reference to the server for fanning role-change updates. */
    explicit GroupInfoHandler(Server& server) : _server(server) {}

protected:
    /**
     * @brief Validate that to (group_id) is present.
     * @throws ProtocolException if the group_id field is missing.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Return group member list with roles; handle grant_admin/revoke_admin sub-commands.
     */
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
