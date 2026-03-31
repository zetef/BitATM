#pragma once
#include <string>

#include "IEntity.h"

/**
 * @brief An authenticated session. Maps to the `sessions` table.
 *
 * Holds the session token and expiry. operator== compares by token.
 */
class Session : public IEntity {
    int _id{0};
    int _userId{0};
    std::string _sessionToken;
    std::string _createdAt;
    std::string _expiresAt;
    bool _isActive{false};

public:
    Session() = default;
    Session(int id, int userId, std::string sessionToken, std::string createdAt,
            std::string expiresAt, bool isActive = true);
    Session(const Session&) = default;
    Session(Session&&) noexcept = default;
    Session& operator=(const Session&) = default;
    Session& operator=(Session&&) noexcept = default;
    ~Session() override = default;

    int getId() const override { return _id; }
    int getUserId() const { return _userId; }
    const std::string& getSessionToken() const { return _sessionToken; }
    const std::string& getCreatedAt() const { return _createdAt; }
    const std::string& getExpiresAt() const { return _expiresAt; }
    bool isActive() const { return _isActive; }

    void deactivate() { _isActive = false; }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
    bool operator==(const IEntity& other) const override;
};
