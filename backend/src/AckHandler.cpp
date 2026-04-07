#include "AckHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "MessageRepository.h"
#include "OfflineQueueRepository.h"

void AckHandler::validate(const Packet& packet) {
    if (packet.body.empty()) throw ProtocolException("ACK: message id (body) is required");
    for (char c : packet.body)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw ProtocolException("ACK: message id must be a positive integer");
}

void AckHandler::execute(Packet& packet, ClientSession& /*session*/) {
    const int messageId = std::stoi(packet.body);

    // Mark message as delivered
    MessageRepository msgRepo;
    auto msgOpt = msgRepo.findById(messageId);
    if (msgOpt) {
        msgOpt->setStatus("delivered");
        msgRepo.save(*msgOpt);
    }

    // Remove from offline_queue (find by message_id, mark delivered)
    OfflineQueueRepository offlineRepo;
    auto entries = offlineRepo.findAll();
    for (auto& entry : entries) {
        if (entry.getMessageId() == messageId && !entry.isDelivered()) {
            offlineRepo.markDelivered(entry.getId());
            break;
        }
    }
}
