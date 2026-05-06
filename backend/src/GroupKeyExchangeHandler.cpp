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
    if (packet.key.empty())
        throw ProtocolException("GROUP_KEY_EXCHANGE: key map (key) is required");
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
    if (actorRole != "creator" && actorRole != "admin")
        throw ProtocolException("GROUP_KEY_EXCHANGE: only creator or admin may distribute keys");

    auto keyPairs = parseKeyPairsGKE(packet.key);
    if (keyPairs.empty())
        throw ProtocolException("GROUP_KEY_EXCHANGE: no valid key pairs found in key field");

    if (keyPairs.size() == 1) {
        // Single-member add
        repo.saveKey(groupId, keyPairs[0].first, keyPairs[0].second);
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
