#include "DeleteConversationHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "MessageRepository.h"
#include "PendingNotificationRepository.h"
#include "Server.h"

void DeleteConversationHandler::validate(const Packet& packet) {
    if (packet.to.empty()) throw ProtocolException("DELETE_CONVERSATION: peer (to) is required");
}

void DeleteConversationHandler::execute(Packet& packet, ClientSession& session) {
    const std::string actor = session.getUsername();
    const std::string peer = packet.to;

    MessageRepository msgRepo;
    msgRepo.deleteConversation(actor, peer);

    auto peerSession = _server.findClient(peer);
    bool delivered = false;
    if (peerSession) {
        Packet notif;
        notif.type = PacketType::DELETE_CONVERSATION;
        notif.from = actor;
        notif.to = peer;
        try {
            peerSession->send(notif);
            delivered = true;
        } catch (const NetworkException&) {
            // peer disconnected between lookup and send - fall through to queuing
        }
    }

    if (!delivered) {
        PendingNotificationRepository pendingRepo;
        pendingRepo.queue(peer, PacketType::DELETE_CONVERSATION, actor);
    }

    // ACK back to the actor so the client knows the delete completed server-side
    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = actor;
    session.send(ack);
}
