#pragma once
#include "BaseAuthHandler.h"

/**
 * @brief Handles GROUP_INFO packets. L3 under BaseAuthHandler.
 *
 * Returns current group membership and roles to the requester.
 * Also handles the "grant_admin" sub-command when body == "grant_admin"
 * (only the group creator may grant admin status).
 */
class GroupInfoHandler : public BaseAuthHandler {
protected:
    /**
     * @brief Validate that to (group_id) is present.
     * @throws ProtocolException if the group_id field is missing.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Return group member list with roles; handle grant_admin sub-command.
     */
    void execute(Packet& packet, ClientSession& session) override;
};
