#include "ReadReceiptHandler.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "DbManager.h"
#include "Server.h"

using namespace Poco::Data::Keywords;

void ReadReceiptHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("READ_RECEIPT: sender (from) is required");
    if (packet.to.empty()) throw ProtocolException("READ_RECEIPT: recipient (to) is required");
    if (packet.body.empty())
        throw ProtocolException("READ_RECEIPT: message timestamp (body) is required");
}

void ReadReceiptHandler::execute(Packet& packet, ClientSession& session) {
    if (packet.from != session.getUsername())
        throw ProtocolException("READ_RECEIPT: sender mismatch with authenticated session");

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
