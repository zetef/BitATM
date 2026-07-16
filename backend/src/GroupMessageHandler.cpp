#include "GroupMessageHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "GroupRepository.h"
#include "Server.h"

void GroupMessageHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("GROUP_MESSAGE: sender (from) is required");
    if (packet.to.empty()) throw ProtocolException("GROUP_MESSAGE: group_id (to) is required");
    if (packet.body.empty()) throw ProtocolException("GROUP_MESSAGE: encrypted body is required");
}

void GroupMessageHandler::execute(Packet& packet, ClientSession& session) {
    int groupId = 0;
    try {
        groupId = std::stoi(packet.to);
    } catch (const std::exception&) {
        throw ProtocolException("GROUP_MESSAGE: invalid group_id in 'to'");
    }

    GroupRepository repo;
    const std::string& senderName = session.getUsername();

    // Verify the sender is a member
    std::string role = repo.getMemberRole(groupId, senderName);
    if (role.empty())
        throw ProtocolException("GROUP_MESSAGE: sender is not a member of group " + packet.to);

    // Persist, then snapshot receipt rows for members-at-send minus sender
    const int msgId = repo.saveMessage(groupId, packet.from, packet.body, packet.timestamp);

    auto members = repo.getMembers(groupId);
    std::vector<std::string> recipients;
    for (const auto& member : members)
        if (member.username != senderName) recipients.push_back(member.username);
    repo.insertReceiptRows(msgId, recipients);

    // Fan out to all online members except the sender
    for (const auto& member : members) {
        if (member.username == senderName) continue;
        auto peerSession = _server.findClient(member.username);
        if (!peerSession) continue;
        try {
            peerSession->send(packet);
        } catch (const NetworkException&) {
            // member disconnected between lookup and send - skip silently
        }
    }

    // ACK sender with the recipient snapshot so the client knows the
    // denominator for delivered-to-all / seen-by-all aggregates
    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    ack.timestamp = packet.timestamp;
    ack.errorMsg = packet.to;  // group id
    std::string joined;
    for (const auto& r : recipients) {
        if (!joined.empty()) joined += ";";
        joined += r;
    }
    ack.key = joined;
    session.send(ack);
}
