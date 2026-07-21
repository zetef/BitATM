#include "DeleteGroupHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "DbManager.h"
#include "GroupRepository.h"
#include "PendingNotificationRepository.h"
#include "Server.h"

void DeleteGroupHandler::validate(const Packet& packet) {
    if (packet.to.empty()) throw ProtocolException("DELETE_GROUP: group_id (to) is required");
}

void DeleteGroupHandler::execute(Packet& packet, ClientSession& session) {
    int groupId = 0;
    try {
        groupId = std::stoi(packet.to);
    } catch (const std::exception&) {
        throw ProtocolException("DELETE_GROUP: invalid group_id in 'to'");
    }

    GroupRepository repo;
    const std::string& actor = session.getUsername();
    std::string role = repo.getMemberRole(groupId, actor);
    if (role.empty()) throw ProtocolException("DELETE_GROUP: actor is not a member of this group");
    if (role != "creator")
        throw ProtocolException("DELETE_GROUP: only the creator may delete the group");

    // Collect all members before deleting so we can notify them
    auto members = repo.getMembers(groupId);

    // Fan GROUP_LEAVE to every online member (including the creator);
    // offline members get it queued for delivery on their next login.
    PendingNotificationRepository pendingRepo;
    for (const auto& m : members) {
        auto peerSession = _server.findClient(m.username);
        if (!peerSession) {
            pendingRepo.queue(m.username, PacketType::GROUP_LEAVE, actor, packet.to);
            continue;
        }
        Packet notif;
        notif.type = PacketType::GROUP_LEAVE;
        notif.to = m.username;
        notif.body = packet.to;  // group_id string
        try {
            peerSession->send(notif);
        } catch (const NetworkException&) {
            // member disconnected between lookup and send - queue instead
            pendingRepo.queue(m.username, PacketType::GROUP_LEAVE, actor, packet.to);
        }
    }

    // Delete the group - CASCADE removes group_members, group_keys, group_messages
    auto ses = DbManager::instance().session();
    ses << "DELETE FROM groups WHERE id = $1", Poco::Data::Keywords::use(groupId),
        Poco::Data::Keywords::now;
}
