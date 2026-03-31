#pragma once
#include <ostream>
#include <string>

#include "IEntity.h"

/**
 * @brief Registered user. Maps to the `users` table.
 *
 * operator== compares by username (case-insensitive).
 * operator<< never prints the password hash.
 */
class User : public IEntity {
    int _id{0};
    std::string _username;
    std::string _passwordHash;
    std::string _publicKey;
    std::string _lastSeen;
    std::string _createdAt;

public:
    User() = default;
    User(int id, std::string username, std::string passwordHash, std::string publicKey = {},
         std::string lastSeen = {}, std::string createdAt = {});
    User(const User&) = default;
    User(User&&) noexcept = default;
    User& operator=(const User&) = default;
    User& operator=(User&&) noexcept = default;
    ~User() override = default;

    int getId() const override { return _id; }
    const std::string& getUsername() const { return _username; }
    const std::string& getPasswordHash() const { return _passwordHash; }
    const std::string& getPublicKey() const { return _publicKey; }
    const std::string& getLastSeen() const { return _lastSeen; }
    const std::string& getCreatedAt() const { return _createdAt; }

    void setPublicKey(const std::string& key) { _publicKey = key; }
    void setLastSeen(const std::string& ts) { _lastSeen = ts; }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
    bool operator==(const IEntity& other) const override;

    friend std::ostream& operator<<(std::ostream& os, const User& u);
};
