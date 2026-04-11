#include "SyncHistoryHandler.h"

#include <algorithm>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "Message.h"
#include "MessageRepository.h"

void SyncHistoryHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("SYNC_HISTORY: sender (from) is required");
}

void SyncHistoryHandler::execute(Packet& packet, ClientSession& session) {
    MessageRepository repo;
    auto messages = repo.findByRecipient(session.getUsername());

    // send in chronological order (operator< compares by created_at)
    std::sort(messages.begin(), messages.end());

    for (const auto& msg : messages) {
        Packet fwd;
        fwd.type = PacketType::MESSAGE;
        fwd.from = msg.getSender();
        fwd.to = session.getUsername();
        fwd.body = msg.getEncryptedBody();
        fwd.key = msg.getEncryptedKey();
        fwd.timestamp = msg.getCreatedAt();
        session.send(fwd);
    }

    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = session.getUsername();
    session.send(ack);
}
