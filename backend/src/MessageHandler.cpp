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
    // Persist message — body is opaque ciphertext, server never decrypts
    Message msg{0, packet.from, packet.to, packet.body, packet.key};
    MessageRepository msgRepo;
    msgRepo.save(msg);

    // Route: online → direct forward, offline → queue
    auto recipient = _server.findClient(packet.to);
    if (recipient) {
        recipient->send(packet);
    } else {
        // Reload to get the generated id (save() used id=0 for insert)
        auto saved = msgRepo.findByRecipient(packet.to);
        if (!saved.empty()) {
            OfflineQueueRepository offlineRepo;
            OfflineMessage entry{0, saved.back().getId(), packet.to};
            offlineRepo.save(entry);
        }
    }

    // ACK back to sender
    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    session.send(ack);
}
