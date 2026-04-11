#pragma once
#include <optional>
#include <string>
#include <vector>

#include "DbManager.h"
#include "IRepository.h"
#include "User.h"

/**
 * @brief PostgreSQL-backed repository for User records.
 *
 * Inherits CRUD from IRepository<User> and adds username lookup.
 * All queries go through DbManager::instance().session().
 */
class UserRepository : public IRepository<User> {
public:
    UserRepository() = default;
    ~UserRepository() = default;

    std::optional<User> findById(int id) override;
    std::vector<User> findAll() override;
    void save(const User& u) override;
    void remove(int id) override;

    /** @brief Find a user by exact username (case-sensitive). */
    std::optional<User> findByUsername(const std::string& username);

    /** @brief Stamp last_seen = NOW() for the given username (called on disconnect). */
    void updateLastSeen(const std::string& username);
};
