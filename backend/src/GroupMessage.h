#pragma once
#include <string>

#include "IEntity.h"

/**
 * @brief A message sent to a group. Maps to the `group_messages` table.
 *
 * encryptedKeys holds a JSON object: { "username": "base64-encrypted-AES-key", ... }
 * so each member can decrypt with their own RSA private key.
 * operator== compares by id.
 */
class GroupMessage : public IEntity {
    int _id{0};
    std::string _groupName;
    std::string _sender;
    std::string _encryptedBody;
    std::string _encryptedKeys;  // JSON: { username -> RSA-encrypted AES key }
    std::string _createdAt;

public:
    GroupMessage() = default;
    GroupMessage(int id, std::string groupName, std::string sender, std::string encryptedBody,
                 std::string encryptedKeys, std::string createdAt = {});
    GroupMessage(const GroupMessage&) = default;
    GroupMessage(GroupMessage&&) noexcept = default;
    GroupMessage& operator=(const GroupMessage&) = default;
    GroupMessage& operator=(GroupMessage&&) noexcept = default;
    ~GroupMessage() override = default;

    int getId() const override { return _id; }
    const std::string& getGroupName() const { return _groupName; }
    const std::string& getSender() const { return _sender; }
    const std::string& getEncryptedKeys() const { return _encryptedKeys; }
    const std::string& getCreatedAt() const { return _createdAt; }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
    bool operator==(const IEntity& other) const override;
};
