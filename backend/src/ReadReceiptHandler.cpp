#include "ReadReceiptHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "Server.h"

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
    }
    // Sender offline: silently drop - read receipts are best-effort
}
