#pragma once
#include <optional>
#include <string>
#include <vector>

#include "DbManager.h"
#include "IRepository.h"
#include "Session.h"

/**
 * @brief PostgreSQL-backed repository for Session records.
 *
 * Inherits CRUD from IRepository<Session> and adds token-based lookup
 * and bulk deactivation (logout).
 */
class SessionRepository : public IRepository<Session> {
public:
    SessionRepository() = default;
    ~SessionRepository() = default;

    std::optional<Session> findById(int id) override;
    std::vector<Session> findAll() override;
    void save(const Session& session) override;
    void remove(int id) override;

    /** @brief Find an active session by its token. Returns nullopt if not found or inactive. */
    std::optional<Session> findByToken(const std::string& token);

    /** @brief Deactivate all sessions belonging to a user (logout everywhere). */
    void deactivateAllForUser(int userId);

    /** @brief Deactivate the session identified by its token (called on WebSocket disconnect). */
    void deactivateByToken(const std::string& token);

    /** @brief Set is_active = FALSE for every session whose expires_at is in the past. */
    void deactivateExpired();
};
