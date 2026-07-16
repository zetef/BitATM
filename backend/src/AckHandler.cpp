#include "AckHandler.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include <algorithm>
#include <cctype>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "DbManager.h"
#include "GroupRepository.h"
#include "MessageRepository.h"
#include "OfflineQueueRepository.h"
#include "Server.h"

using namespace Poco::Data::Keywords;

namespace {
bool isNumeric(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}
}  // namespace

void AckHandler::validate(const Packet& packet) {
    if (packet.errorMsg == "delivered") {
        // Group delivered receipt: to = numeric group id, body = message ts
        if (!isNumeric(packet.to))
            throw ProtocolException("ACK: delivered receipt needs numeric group id in 'to'");
        if (packet.body.empty())
            throw ProtocolException("ACK: delivered receipt needs message timestamp in body");
        return;
    }
    if (packet.body.empty()) throw ProtocolException("ACK: message id (body) is required");
    for (char c : packet.body)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw ProtocolException("ACK: message id must be a positive integer");
}

void AckHandler::execute(Packet& packet, ClientSession& session) {
    if (packet.errorMsg == "delivered") {
        executeGroupDelivered(packet, session);
        return;
    }

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

void AckHandler::executeGroupDelivered(Packet& packet, ClientSession& session) {
    const int groupId = std::stoi(packet.to);
    const std::string& member = session.getUsername();

    GroupRepository repo;
    if (repo.getMemberRole(groupId, member).empty())
        throw ProtocolException("ACK: acker is not a member of group " + packet.to);

    const auto senders = repo.markReceiptDelivered(groupId, packet.body, member);

    Packet fwd;
    fwd.type = PacketType::ACK;
    fwd.from = member;
    fwd.to = packet.to;  // group id - client routes by conversation
    fwd.body = packet.body;
    fwd.errorMsg = "delivered";

    for (const auto& sender : senders) {
        if (sender == member) continue;
        auto peer = _server.findClient(sender);
        if (peer) {
            try {
                peer->send(fwd);
            } catch (const NetworkException&) {
            }
            continue;
        }
        try {
            auto ses = DbManager::instance().session();
            std::string fromUser = member;
            std::string toUser = sender;
            std::string msgTs = packet.body;
            int gid = groupId;
            std::string kind = "delivered";
            // clang-format off
            ses << "INSERT INTO offline_read_receipts (from_user, to_user, message_ts, group_id, kind) "
                   "VALUES ($1, $2, $3, $4, $5) ON CONFLICT DO NOTHING",
                use(fromUser), use(toUser), use(msgTs), use(gid), use(kind), now;
            // clang-format on
        } catch (const Poco::Exception& e) {
            throw DbException("AckHandler: delivered queue failed: " + e.message());
        }
    }
}
