#include "SyncHistoryHandler.h"

#include <Poco/Logger.h>

#include <algorithm>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "GroupRepository.h"
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
    } catch (const DbException& e) {
        // cursor was not a valid timestamp - fall back to full history
        poco_warning(Poco::Logger::get("SyncHistoryHandler"),
                     "message cursor query failed, replaying full history for " + username + ": " +
                         e.what());
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

    // Replay missed group messages; GroupMessageHandler skips offline members
    // at send time, so sync is the only delivery path after a reconnect.
    GroupRepository groupRepo;
    std::vector<GroupMessageRow> groupMessages;
    try {
        groupMessages = groupRepo.findMessagesForUserSince(username, cursor);
    } catch (const DbException& e) {
        poco_warning(
            Poco::Logger::get("SyncHistoryHandler"),
            "group cursor query failed, replaying full history for " + username + ": " + e.what());
        groupMessages = groupRepo.findMessagesForUserSince(username, "");
    }

    for (const auto& gm : groupMessages) {
        Packet fwd;
        fwd.type = PacketType::GROUP_MESSAGE;
        fwd.from = gm.sender;
        fwd.to = std::to_string(gm.groupId);
        fwd.body = gm.encryptedBody;
        fwd.timestamp = gm.timestamp;
        session.send(fwd);
    }

    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = username;
    session.send(ack);
}
