#include "CreateGroupHandler.h"

#include <sstream>
#include <unordered_map>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "GroupRepository.h"
#include "PendingNotificationRepository.h"
#include "Server.h"
#include "UserRepository.h"

// ---------------------------------------------------------------------------
// File-local helpers
// ---------------------------------------------------------------------------

/** @brief Parse a comma-separated member list into a vector of usernames. */
static std::vector<std::string> parseMembers(const std::string& body) {
    std::vector<std::string> members;
    std::istringstream ss(body);
    std::string m;
    while (std::getline(ss, m, ',')) {
        if (!m.empty()) members.push_back(m);
    }
    return members;
}

/**
 * @brief Parse a pipe-separated key map into (username, encryptedKey) pairs.
 *
 * Format: "user1:key1;user2:key2;..."
 */
static std::vector<std::pair<std::string, std::string>> parseKeyPairs(const std::string& keyField) {
    std::vector<std::pair<std::string, std::string>> pairs;
    std::istringstream ss(keyField);
    std::string entry;
    while (std::getline(ss, entry, ';')) {
        auto colonPos = entry.find(':');
        if (colonPos == std::string::npos || colonPos == 0) continue;
        std::string user = entry.substr(0, colonPos);
        std::string key = entry.substr(colonPos + 1);
        if (!user.empty() && !key.empty()) pairs.emplace_back(user, key);
    }
    return pairs;
}

// ---------------------------------------------------------------------------
// CreateGroupHandler
// ---------------------------------------------------------------------------

void CreateGroupHandler::validate(const Packet& packet) {
    if (packet.errorMsg.empty())
        throw ProtocolException("CREATE_GROUP: group name (errorMsg) is required");
    if (packet.body.empty())
        throw ProtocolException("CREATE_GROUP: member list (body) is required");
    if (packet.key.empty()) throw ProtocolException("CREATE_GROUP: key map (key) is required");

    auto members = parseMembers(packet.body);
    // total = listed members + creator
    std::size_t total = members.size() + 1;
    if (total < 2 || total > 32)
        throw ProtocolException("CREATE_GROUP: group size must be between 2 and 32 (inclusive)");
}

void CreateGroupHandler::execute(Packet& packet, ClientSession& session) {
    const std::string& creator = session.getUsername();
    const std::string& groupName = packet.errorMsg;

    auto members = parseMembers(packet.body);
    auto keyPairs = parseKeyPairs(packet.key);

    // Verify every invited member exists
    UserRepository userRepo;
    for (const auto& m : members) {
        if (!userRepo.findByUsername(m))
            throw ProtocolException("CREATE_GROUP: user not found: " + m);
    }

    // Build a lookup map for keys: username -> encryptedKey
    std::unordered_map<std::string, std::string> keyMap;
    for (const auto& [user, key] : keyPairs) keyMap[user] = key;

    // Persist group and memberships
    GroupRepository repo;
    int groupId = repo.createGroup(groupName, creator);
    repo.addMember(groupId, creator, "creator");
    for (const auto& m : members) repo.addMember(groupId, m, "member");

    // Persist keys
    for (const auto& [user, key] : keyPairs) repo.saveKey(groupId, user, key);

    const std::string groupIdStr = std::to_string(groupId);

    // Fan GROUP_INVITE to each member. An offline member gets it queued for
    // delivery on their next login - previously silently dropped, meaning an
    // offline invitee at group-creation time would never learn about the
    // group at all (unlike the single-member-add path, which already queues).
    PendingNotificationRepository pendingRepo;
    for (const auto& m : members) {
        std::string key;
        auto keyIt = keyMap.find(m);
        if (keyIt != keyMap.end()) key = keyIt->second;

        auto peerSession = _server.findClient(m);
        if (!peerSession) {
            pendingRepo.queue(m, PacketType::GROUP_INVITE, creator, groupIdStr, groupName, key);
            continue;
        }

        Packet invite;
        invite.type = PacketType::GROUP_INVITE;
        invite.from = creator;
        invite.to = m;
        invite.errorMsg = groupName;
        invite.body = groupIdStr;
        invite.key = key;
        try {
            peerSession->send(invite);
        } catch (const NetworkException&) {
            // member disconnected between lookup and send - queue instead
            pendingRepo.queue(m, PacketType::GROUP_INVITE, creator, groupIdStr, groupName, key);
        }
    }

    // Build member list string for GROUP_INFO response: "user:role;..."
    // Semicolon avoids collision with the pipe wire-format delimiter
    std::string memberList = creator + ":creator";
    for (const auto& m : members) memberList += ";" + m + ":member";

    // Reply to creator with GROUP_INFO
    Packet info;
    info.type = PacketType::GROUP_INFO;
    info.from = creator;
    info.to = creator;
    info.body = groupIdStr;
    info.errorMsg = groupName;
    info.key = memberList;
    session.send(info);
}
