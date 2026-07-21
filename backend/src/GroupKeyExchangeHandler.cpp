#include "GroupKeyExchangeHandler.h"

#include <sstream>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "GroupRepository.h"
#include "Server.h"

/** @brief Parse a pipe-separated key map into (username, encryptedKey) pairs. */
static std::vector<std::pair<std::string, std::string>> parseKeyPairsGKE(
    const std::string& keyField) {
    std::vector<std::pair<std::string, std::string>> pairs;
    std::istringstream ss(keyField);
    std::string entry;
    while (std::getline(ss, entry, '|')) {
        auto colonPos = entry.find(':');
        if (colonPos == std::string::npos || colonPos == 0) continue;
        std::string user = entry.substr(0, colonPos);
        std::string key = entry.substr(colonPos + 1);
        if (!user.empty() && !key.empty()) pairs.emplace_back(user, key);
    }
    return pairs;
}

void GroupKeyExchangeHandler::validate(const Packet& packet) {
    if (packet.to.empty()) throw ProtocolException("GROUP_KEY_EXCHANGE: group_id (to) is required");
    // Empty key field is allowed: it means "resend me my own stored key"
}

void GroupKeyExchangeHandler::execute(Packet& packet, ClientSession& session) {
    int groupId = 0;
    try {
        groupId = std::stoi(packet.to);
    } catch (const std::exception&) {
        throw ProtocolException("GROUP_KEY_EXCHANGE: invalid group_id in 'to'");
    }

    GroupRepository repo;
    const std::string& actorName = session.getUsername();
    std::string actorRole = repo.getMemberRole(groupId, actorName);

    // Key recovery: any member may request their own stored wrapped key
    // (lost local cache, new device). Reply mirrors the distribution shape.
    if (packet.key.empty()) {
        if (actorRole.empty())
            throw ProtocolException("GROUP_KEY_EXCHANGE: requester is not a member of group " +
                                    packet.to);
        auto stored = repo.getKey(groupId, actorName);
        if (!stored)
            throw ProtocolException("GROUP_KEY_EXCHANGE: no stored key for requester in group " +
                                    packet.to);
        Packet resp;
        resp.type = PacketType::GROUP_KEY_EXCHANGE;
        resp.from = actorName;
        resp.to = packet.to;
        resp.key = *stored;
        resp.body = actorName;
        session.send(resp);
        return;
    }

    if (actorRole != "creator" && actorRole != "admin")
        throw ProtocolException("GROUP_KEY_EXCHANGE: only creator or admin may distribute keys");

    auto keyPairs = parseKeyPairsGKE(packet.key);
    if (keyPairs.empty())
        throw ProtocolException("GROUP_KEY_EXCHANGE: no valid key pairs found in key field");

    if (keyPairs.size() == 1) {
        // Single-member add. Only insert group_members if they aren't already
        // a member - this path is also reachable in principle for a lone key
        // refresh, and re-adding must never demote an existing admin/creator.
        const std::string& newMember = keyPairs[0].first;
        if (repo.getMemberRole(groupId, newMember).empty())
            repo.addMember(groupId, newMember, "member");
        repo.saveKey(groupId, newMember, keyPairs[0].second);
    } else {
        // Full rotation after kick
        repo.replaceAllKeys(groupId, keyPairs);
    }

    // Notify each affected member with their individual key
    for (const auto& [user, key] : keyPairs) {
        auto peerSession = _server.findClient(user);
        if (!peerSession) continue;

        Packet resp;
        resp.type = PacketType::GROUP_KEY_EXCHANGE;
        resp.from = actorName;
        resp.to = packet.to;  // group_id as string
        resp.key = key;
        resp.body = user;  // recipient username so client can verify
        try {
            peerSession->send(resp);
        } catch (const NetworkException&) {
            // recipient disconnected - skip silently
        }
    }
}
