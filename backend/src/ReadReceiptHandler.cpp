#include "ReadReceiptHandler.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include <algorithm>
#include <cctype>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "DbManager.h"
#include "GroupRepository.h"
#include "Server.h"

using namespace Poco::Data::Keywords;

namespace {
bool isNumericGroupId(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}
}  // namespace

void ReadReceiptHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("READ_RECEIPT: sender (from) is required");
    if (packet.to.empty()) throw ProtocolException("READ_RECEIPT: recipient (to) is required");
    if (packet.body.empty())
        throw ProtocolException("READ_RECEIPT: message timestamp (body) is required");
}

void ReadReceiptHandler::execute(Packet& packet, ClientSession& session) {
    if (packet.from != session.getUsername())
        throw ProtocolException("READ_RECEIPT: sender mismatch with authenticated session");

    // Group receipt: 'to' is a numeric group id. Fan out to every member
    // except the reader; queue for offline members with the group id kept.
    if (isNumericGroupId(packet.to)) {
        const int groupId = std::stoi(packet.to);
        GroupRepository repo;
        if (repo.getMemberRole(groupId, packet.from).empty())
            throw ProtocolException("READ_RECEIPT: reader is not a member of group " + packet.to);

        // v2: record per-member seen (implies delivered) on the receipt rows.
        // Fan-out below stays: non-sender members ignore it, the sender's
        // client recomputes its aggregate from the row update.
        repo.markReceiptSeen(groupId, packet.body, packet.from);

        for (const auto& member : repo.getMembers(groupId)) {
            if (member.username == packet.from) continue;
            auto peer = _server.findClient(member.username);
            if (peer) {
                try {
                    peer->send(packet);
                } catch (const NetworkException&) {
                }
                continue;
            }
            try {
                auto ses = DbManager::instance().session();
                std::string fromUser = packet.from;
                std::string toUser = member.username;
                std::string msgTs = packet.body;
                int gid = groupId;
                // clang-format off
                ses << "INSERT INTO offline_read_receipts (from_user, to_user, message_ts, group_id) "
                       "VALUES ($1, $2, $3, $4) "
                       "ON CONFLICT DO NOTHING",
                    use(fromUser), use(toUser), use(msgTs), use(gid), now;
                // clang-format on
            } catch (const Poco::Exception& e) {
                throw DbException("ReadReceiptHandler: group queue failed: " + e.message());
            }
        }
        return;
    }

    auto sender = _server.findClient(packet.to);
    if (sender) {
        sender->send(packet);
    } else {
        // Original sender is offline - queue for delivery on their next login.
        try {
            auto ses = DbManager::instance().session();
            std::string fromUser = packet.from;
            std::string toUser = packet.to;
            std::string msgTs = packet.body;
            // clang-format off
            ses << "INSERT INTO offline_read_receipts (from_user, to_user, message_ts) "
                   "VALUES ($1, $2, $3) "
                   "ON CONFLICT DO NOTHING",
                use(fromUser), use(toUser), use(msgTs), now;
            // clang-format on
        } catch (const Poco::Exception& e) {
            throw DbException("ReadReceiptHandler: queue failed: " + e.message());
        }
    }

    // Fan-out READ_RECEIPT to all sibling sessions of the reader (multi-device sync)
    auto readerSessions = _server.getSessionsForUser(packet.from);
    for (auto& sibling : readerSessions) {
        if (sibling.get() != &session) {
            try {
                sibling->send(packet);
            } catch (const NetworkException&) {
                // sibling disconnected between session copy and send
            }
        }
    }
}
