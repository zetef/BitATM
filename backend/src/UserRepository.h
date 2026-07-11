#pragma once
#include <optional>
#include <string>
#include <vector>

#include "BaseSqlRepository.h"
#include "User.h"

/**
 * @brief PostgreSQL-backed repository for User records.
 *
 * Inherits CRUD via BaseSqlRepository<User> (shared session plumbing, generic remove()) and adds
 * username lookup. All queries go through DbManager::instance().session().
 */
class UserRepository : public BaseSqlRepository<User> {
public:
    UserRepository() : BaseSqlRepository("users") {}
    ~UserRepository() = default;

    std::optional<User> findById(int id) override;
    std::vector<User> findAll() override;
    void save(const User& u) override;

    /** @brief Find a user by exact username (case-sensitive). */
    std::optional<User> findByUsername(const std::string& username);

    /** @brief Stamp last_seen = NOW() for the given username (called on disconnect). */
    void updateLastSeen(const std::string& username);
};
