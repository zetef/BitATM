#include "MessageHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "Message.h"
#include "MessageRepository.h"
#include "OfflineQueueRepository.h"
#include "Server.h"

void MessageHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("MESSAGE: sender (from) is required");
    if (packet.to.empty()) throw ProtocolException("MESSAGE: recipient (to) is required");
    if (packet.body.empty()) throw ProtocolException("MESSAGE: encrypted body is required");
    if (packet.key.empty()) throw ProtocolException("MESSAGE: encrypted key is required");
}

void MessageHandler::execute(Packet& packet, ClientSession& session) {
    // Parse dual-key field: "recipientKey;senderKey" or plain "recipientKey"
    std::string recipientKey = packet.key;
    std::string senderKey;
    const auto sep = packet.key.find(';');
    if (sep != std::string::npos) {
        recipientKey = packet.key.substr(0, sep);
        senderKey = packet.key.substr(sep + 1);
    }

    // Persist message - body is opaque ciphertext, server never decrypts
    Message msg{0, packet.from, packet.to, packet.body, recipientKey, senderKey};
    MessageRepository msgRepo;
    msgRepo.save(msg);

    // Route to recipient: forward only their key segment
    auto recipient = _server.findClient(packet.to);
    if (recipient) {
        Packet fwd = packet;
        fwd.key = recipientKey;
        recipient->send(fwd);
    } else {
        // Reload to get the generated id (save() used id=0 for insert)
        auto saved = msgRepo.findByRecipient(packet.to);
        if (!saved.empty()) {
            OfflineQueueRepository offlineRepo;
            OfflineMessage entry{0, saved.back().getId(), packet.to, ""};
            offlineRepo.save(entry);
        }
    }

    // Fan-out to sender's other active sessions (multi-device echo)
    // Keep full key field so sibling can extract senderKey segment
    // errorMsg="1" signals to sibling that this is an outgoing copy
    auto senderSessions = _server.getSessionsForUser(packet.from);
    for (auto& sibling : senderSessions) {
        if (sibling.get() != &session) {
            try {
                Packet echo = packet;  // packet.key still has both segments
                echo.errorMsg = "1";
                sibling->send(echo);
            } catch (const NetworkException&) {
                // sibling disconnected between session copy and send
            }
        }
    }

    // ACK back to sender
    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    ack.timestamp = packet.timestamp;
    session.send(ack);
}
