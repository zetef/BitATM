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
    const std::string& username = session.getUsername();
    // client sends newest known timestamp in body as cursor; empty = all history
    const std::string cursor = packet.body;

    MessageRepository repo;
    std::vector<Message> messages;
    try {
        messages = repo.findAllForUser(username, cursor);
    } catch (const DbException&) {
        // cursor was not a valid timestamp - fall back to full history
        messages = repo.findAllForUser(username, "");
    }

    // send in chronological order (operator< compares by created_at)
    std::sort(messages.begin(), messages.end());

    for (const auto& msg : messages) {
        Packet fwd;
        fwd.type = PacketType::MESSAGE;
        fwd.from = msg.getSender();
        fwd.to = msg.getRecipient();
        fwd.body = msg.getEncryptedBody();
        fwd.timestamp = msg.getCreatedAt();

        const bool isOutgoing = (msg.getSender() == username);
        fwd.errorMsg = isOutgoing ? "1" : "0";

        if (isOutgoing && !msg.getSenderEncryptedKey().empty()) {
            // Send sender's own key copy so the sibling device can decrypt
            fwd.key = msg.getSenderEncryptedKey();
        } else {
            // Incoming message or legacy row (no sender key stored): send recipient key
            // Frontend falls back to "[Sent]" if it cannot decrypt
            fwd.key = msg.getEncryptedKey();
        }

        session.send(fwd);
    }

    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = username;
    session.send(ack);
}
