#include "GroupRepository.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/Transaction.h>

#include "../../common/AppException.h"

using namespace Poco::Data::Keywords;

int GroupRepository::createGroup(const std::string& name, const std::string& creator) {
    try {
        auto ses = DbManager::instance().session();
        int id = 0;
        std::string nameCopy = name;
        std::string creatorCopy = creator;
        // clang-format off
        ses << "INSERT INTO groups(name, creator) VALUES($1, $2) RETURNING id",
            use(nameCopy), use(creatorCopy), into(id), now;
        // clang-format on
        return id;
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::createGroup: " + e.message());
    }
}

std::optional<GroupInfo> GroupRepository::findGroupById(int groupId) {
    try {
        auto ses = DbManager::instance().session();
        int id = 0;
        std::string name;
        std::string creator;
        // clang-format off
        ses << "SELECT id, name, creator FROM groups WHERE id = $1",
            into(id), into(name), into(creator), use(groupId), now;
        // clang-format on
        if (name.empty()) return std::nullopt;
        return GroupInfo{id, name, creator};
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::findGroupById: " + e.message());
    }
}

void GroupRepository::addMember(int groupId, const std::string& username, const std::string& role) {
    try {
        auto ses = DbManager::instance().session();
        std::string usernameCopy = username;
        std::string roleCopy = role;
        // clang-format off
        ses << "INSERT INTO group_members(group_id, username, role) VALUES($1, $2, $3) "
               "ON CONFLICT (group_id, username) DO UPDATE SET role = EXCLUDED.role",
            use(groupId), use(usernameCopy), use(roleCopy), now;
        // clang-format on
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::addMember: " + e.message());
    }
}

void GroupRepository::removeMember(int groupId, const std::string& username) {
    try {
        auto ses = DbManager::instance().session();
        std::string usernameCopy = username;
        // clang-format off
        ses << "DELETE FROM group_keys WHERE group_id = $1 AND username = $2",
            use(groupId), use(usernameCopy), now;
        ses << "DELETE FROM group_members WHERE group_id = $1 AND username = $2",
            use(groupId), use(usernameCopy), now;
        // clang-format on
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::removeMember: " + e.message());
    }
}

std::vector<GroupMember> GroupRepository::getMembers(int groupId) {
    try {
        auto ses = DbManager::instance().session();
        std::vector<GroupMember> members;
        std::string username;
        std::string role;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT username, role FROM group_members WHERE group_id = $1 ORDER BY joined_at",
            into(username), into(role), use(groupId), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!username.empty()) {
                members.push_back(GroupMember{groupId, username, role});
                username.clear();
                role.clear();
            }
        }
        return members;
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::getMembers: " + e.message());
    }
}

std::string GroupRepository::getMemberRole(int groupId, const std::string& username) {
    try {
        auto ses = DbManager::instance().session();
        std::string role;
        std::string usernameCopy = username;
        // clang-format off
        ses << "SELECT role FROM group_members WHERE group_id = $1 AND username = $2",
            into(role), use(groupId), use(usernameCopy), now;
        // clang-format on
        return role;
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::getMemberRole: " + e.message());
    }
}

void GroupRepository::saveKey(int groupId, const std::string& username,
                              const std::string& encryptedKey) {
    try {
        auto ses = DbManager::instance().session();
        std::string usernameCopy = username;
        std::string keyCopy = encryptedKey;
        // clang-format off
        ses << "INSERT INTO group_keys(group_id, username, encrypted_aes_key) VALUES($1, $2, $3) "
               "ON CONFLICT (group_id, username) DO UPDATE SET encrypted_aes_key = EXCLUDED.encrypted_aes_key",
            use(groupId), use(usernameCopy), use(keyCopy), now;
        // clang-format on
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::saveKey: " + e.message());
    }
}

void GroupRepository::replaceAllKeys(
    int groupId, const std::vector<std::pair<std::string, std::string>>& userKeyPairs) {
    try {
        auto ses = DbManager::instance().session();
        Poco::Data::Transaction trx(ses);
        ses << "DELETE FROM group_keys WHERE group_id = $1", use(groupId), now;
        for (const auto& [username, encryptedKey] : userKeyPairs) {
            std::string usernameCopy = username;
            std::string keyCopy = encryptedKey;
            // clang-format off
            ses << "INSERT INTO group_keys(group_id, username, encrypted_aes_key) VALUES($1, $2, $3)",
                use(groupId), use(usernameCopy), use(keyCopy), now;
            // clang-format on
        }
        trx.commit();
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::replaceAllKeys: " + e.message());
    }
}

std::optional<std::string> GroupRepository::getKey(int groupId, const std::string& username) {
    try {
        auto ses = DbManager::instance().session();
        std::string encryptedKey;
        std::string usernameCopy = username;
        // clang-format off
        ses << "SELECT encrypted_aes_key FROM group_keys WHERE group_id = $1 AND username = $2",
            into(encryptedKey), use(groupId), use(usernameCopy), now;
        // clang-format on
        if (encryptedKey.empty()) return std::nullopt;
        return encryptedKey;
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::getKey: " + e.message());
    }
}

int GroupRepository::saveMessage(int groupId, const std::string& sender,
                                 const std::string& encryptedBody, const std::string& timestamp) {
    try {
        auto ses = DbManager::instance().session();
        int id = 0;
        std::string senderCopy = sender;
        std::string bodyCopy = encryptedBody;
        std::string tsCopy = timestamp;
        // clang-format off
        ses << "INSERT INTO group_messages(group_id, sender, encrypted_body, timestamp) "
               "VALUES($1, $2, $3, $4::timestamptz) RETURNING id",
            use(groupId), use(senderCopy), use(bodyCopy), use(tsCopy), into(id), now;
        // clang-format on
        return id;
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::saveMessage: " + e.message());
    }
}

int GroupRepository::memberCount(int groupId) {
    try {
        auto ses = DbManager::instance().session();
        int count = 0;
        ses << "SELECT COUNT(*) FROM group_members WHERE group_id = $1", into(count), use(groupId),
            now;
        return count;
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::memberCount: " + e.message());
    }
}

void GroupRepository::updateRole(int groupId, const std::string& username,
                                 const std::string& newRole) {
    try {
        auto ses = DbManager::instance().session();
        std::string usernameCopy = username;
        std::string roleCopy = newRole;
        // clang-format off
        ses << "UPDATE group_members SET role = $1 WHERE group_id = $2 AND username = $3",
            use(roleCopy), use(groupId), use(usernameCopy), now;
        // clang-format on
    } catch (const Poco::Exception& e) {
        throw DbException("GroupRepository::updateRole: " + e.message());
    }
}
