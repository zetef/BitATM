#include "GroupLeaveHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "GroupRepository.h"
#include "Server.h"

void GroupLeaveHandler::validate(const Packet& packet) {
    if (packet.to.empty()) throw ProtocolException("GROUP_LEAVE: group_id (to) is required");
}

void GroupLeaveHandler::execute(Packet& packet, ClientSession& session) {
    int groupId = 0;
    try {
        groupId = std::stoi(packet.to);
    } catch (const std::exception&) {
        throw ProtocolException("GROUP_LEAVE: invalid group_id in 'to'");
    }

    GroupRepository repo;
    const std::string& actorName = session.getUsername();
    std::string actorRole = repo.getMemberRole(groupId, actorName);
    if (actorRole.empty())
        throw ProtocolException("GROUP_LEAVE: actor is not a member of group " + packet.to);

    bool isKick = !packet.body.empty();
    std::string target = isKick ? packet.body : actorName;

    // Creator cannot leave or be kicked
    std::string targetRole = repo.getMemberRole(groupId, target);
    if (targetRole == "creator")
        throw ProtocolException("GROUP_LEAVE: creator cannot leave or be kicked");

    if (isKick) {
        // Only creator can kick an admin; any admin/creator can kick a member
        if (targetRole == "admin" && actorRole != "creator")
            throw ProtocolException("GROUP_LEAVE: only the creator can kick an admin");
        // Actor must be at least admin to kick
        if (actorRole != "creator" && actorRole != "admin")
            throw ProtocolException("GROUP_LEAVE: only creator or admin can kick members");
    }

    repo.removeMember(groupId, target);

    // Notify the target if they are online
    auto targetSession = _server.findClient(target);
    if (targetSession) {
        Packet leaveNotif;
        leaveNotif.type = PacketType::GROUP_LEAVE;
        leaveNotif.to = target;
        leaveNotif.body = packet.to;  // group_id
        try {
            targetSession->send(leaveNotif);
        } catch (const NetworkException&) {
            // target disconnected - skip silently
        }
    }

    // Signal to actor that key rotation is needed after a kick
    if (isKick) {
        Packet rotateSignal;
        rotateSignal.type = PacketType::GROUP_LEAVE;
        rotateSignal.to = actorName;
        rotateSignal.body = "rotate";
        rotateSignal.key = packet.to;  // group_id so client knows which group
        session.send(rotateSignal);
    }
}
