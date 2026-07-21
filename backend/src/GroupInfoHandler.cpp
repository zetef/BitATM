#include "GroupInfoHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "GroupRepository.h"

void GroupInfoHandler::validate(const Packet& packet) {
    if (packet.to.empty()) throw ProtocolException("GROUP_INFO: group_id (to) is required");
}

void GroupInfoHandler::execute(Packet& packet, ClientSession& session) {
    int groupId = 0;
    try {
        groupId = std::stoi(packet.to);
    } catch (const std::exception&) {
        throw ProtocolException("GROUP_INFO: invalid group_id in 'to'");
    }

    GroupRepository repo;
    const std::string& requester = session.getUsername();

    // Handle grant_admin sub-command
    if (packet.body == "grant_admin") {
        std::string requesterRole = repo.getMemberRole(groupId, requester);
        if (requesterRole != "creator")
            throw ProtocolException("GROUP_INFO: only the creator may grant admin status");
        if (packet.key.empty())
            throw ProtocolException(
                "GROUP_INFO: target username (key) is required for grant_admin");
        repo.updateRole(groupId, packet.key, "admin");
    }

    // Handle revoke_admin sub-command
    if (packet.body == "revoke_admin") {
        std::string requesterRole = repo.getMemberRole(groupId, requester);
        if (requesterRole != "creator")
            throw ProtocolException("GROUP_INFO: only the creator may revoke admin status");
        if (packet.key.empty())
            throw ProtocolException(
                "GROUP_INFO: target username (key) is required for revoke_admin");
        if (repo.getMemberRole(groupId, packet.key) != "admin")
            throw ProtocolException("GROUP_INFO: target is not an admin");
        repo.updateRole(groupId, packet.key, "member");
    }

    auto group = repo.findGroupById(groupId);
    if (!group) throw ProtocolException("GROUP_INFO: group not found: " + packet.to);

    auto members = repo.getMembers(groupId);
    std::string memberList;
    for (const auto& m : members) {
        if (!memberList.empty()) memberList += ";";
        memberList += m.username + ":" + m.role;
    }

    Packet resp;
    resp.type = PacketType::GROUP_INFO;
    resp.from = requester;
    resp.to = requester;
    resp.body = packet.to;  // group_id as string
    resp.errorMsg = group->name;
    resp.key = memberList;
    session.send(resp);
}
