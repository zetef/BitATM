#pragma once
#include "BaseAuthHandler.h"

class Server;

/**
 * @brief Handles DELETE_CONVERSATION packets ("delete for everyone"). L3 under BaseAuthHandler.
 *
 * Hard-deletes all messages between the sender and the peer named in `to`.
 * The peer is notified immediately if online; otherwise a pending_notifications
 * row is queued and delivered as the same packet on the peer's next login.
 */
class DeleteConversationHandler : public BaseAuthHandler {
public:
    /** @brief Construct with a reference to the server for client lookup. */
    explicit DeleteConversationHandler(Server& server) : _server(server) {}

protected:
    /**
     * @brief Validate that to (peer username) is present.
     * @throws ProtocolException if the peer field is missing.
     */
    void validate(const Packet& packet) override;

    /**
     * @brief Delete the conversation's messages, then notify the peer now or on next login.
     */
    void execute(Packet& packet, ClientSession& session) override;

private:
    Server& _server;
};
