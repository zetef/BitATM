#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "DbManager.h"

/**
 * @brief Metadata for a group returned by GroupRepository::findGroupById.
 */
struct GroupInfo {
    int id = 0;
    std::string name;
    std::string creator;
};

/**
 * @brief A single member entry returned by GroupRepository::getMembers.
 */
struct GroupMember {
    int groupId = 0;
    std::string username;
    std::string role;  // "creator" | "admin" | "member"
};

/**
 * @brief PostgreSQL-backed repository for groups, group_members, group_keys, group_messages.
 *
 * This class is NOT an IRepository<T> template because it spans four tables
 * and does not map to a single IEntity subclass.
 */
class GroupRepository {
public:
    GroupRepository() = default;

    /**
     * @brief Insert a new group row.
     * @return The auto-generated group id.
     */
    int createGroup(const std::string& name, const std::string& creator);

    /**
     * @brief Fetch group metadata by id.
     * @return nullopt if no group with that id exists.
     */
    std::optional<GroupInfo> findGroupById(int groupId);

    /**
     * @brief Add a member to a group with the given role.
     * @param role One of "creator", "admin", or "member".
     */
    void addMember(int groupId, const std::string& username, const std::string& role);

    /**
     * @brief Remove a member from a group.
     *
     * Also deletes the corresponding group_keys row for that member.
     */
    void removeMember(int groupId, const std::string& username);

    /**
     * @brief Return all members of a group.
     */
    std::vector<GroupMember> getMembers(int groupId);

    /**
     * @brief Return the role of a user in a group.
     * @return Role string, or empty string if the user is not a member.
     */
    std::string getMemberRole(int groupId, const std::string& username);

    /**
     * @brief Store the encrypted AES key for one member.
     *
     * Upserts: inserts a new row or updates the existing one on conflict.
     */
    void saveKey(int groupId, const std::string& username, const std::string& encryptedKey);

    /**
     * @brief Atomically replace ALL keys for a group.
     *
     * Used for key rotation after a member is kicked. Deletes every existing
     * group_keys row for the group and inserts the new set in a single transaction.
     * @param userKeyPairs Vector of (username, encryptedKey) pairs.
     */
    void replaceAllKeys(int groupId,
                        const std::vector<std::pair<std::string, std::string>>& userKeyPairs);

    /**
     * @brief Return the encrypted AES key for a specific member.
     * @return nullopt if no key is stored for that member.
     */
    std::optional<std::string> getKey(int groupId, const std::string& username);

    /**
     * @brief Persist a group message.
     * @return The auto-generated message id.
     */
    int saveMessage(int groupId, const std::string& sender, const std::string& encryptedBody,
                    const std::string& timestamp);

    /**
     * @brief Return the number of members in a group.
     */
    int memberCount(int groupId);

    /**
     * @brief Update a member's role.
     */
    void updateRole(int groupId, const std::string& username, const std::string& newRole);
};
