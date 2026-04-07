#include "SessionRepository.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include "../../common/AppException.h"

using namespace Poco::Data::Keywords;

namespace {
::Session rowToSession(int id, int userId, const std::string& token, const std::string& createdAt,
                       const std::string& expiresAt, bool isActive) {
    return ::Session{id, userId, token, createdAt, expiresAt, isActive};
}
}  // namespace

std::optional<::Session> SessionRepository::findById(int id) {
    try {
        auto ses = DbManager::instance().session();
        int sid, userId;
        std::string token, createdAt, expiresAt;
        int isActive;
        // clang-format off
        ses << "SELECT id, user_id, session_token, created_at, expires_at, is_active "
               "FROM sessions WHERE id = ?",
            into(sid), into(userId), into(token),
            into(createdAt), into(expiresAt), into(isActive),
            use(id), now;
        // clang-format on
        if (token.empty()) return std::nullopt;
        return rowToSession(sid, userId, token, createdAt, expiresAt, isActive != 0);
    } catch (const Poco::Exception& e) {
        throw DbException("SessionRepository::findById: " + e.message());
    }
}

std::vector<::Session> SessionRepository::findAll() {
    try {
        auto ses = DbManager::instance().session();
        std::vector<::Session> sessions;
        int sid, userId, isActive;
        std::string token, createdAt, expiresAt;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, user_id, session_token, created_at, expires_at, is_active FROM sessions",
            into(sid), into(userId), into(token),
            into(createdAt), into(expiresAt), into(isActive), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!token.empty())
                sessions.push_back(
                    rowToSession(sid, userId, token, createdAt, expiresAt, isActive != 0));
        }
        return sessions;
    } catch (const Poco::Exception& e) {
        throw DbException("SessionRepository::findAll: " + e.message());
    }
}

void SessionRepository::save(const ::Session& s) {
    try {
        auto ses = DbManager::instance().session();
        if (s.getId() == 0) {
            int userId = s.getUserId();
            std::string token = s.getSessionToken();
            std::string expires = s.getExpiresAt();
            // clang-format off
            ses << "INSERT INTO sessions (user_id, session_token, expires_at) VALUES (?, ?, ?)",
                use(userId), use(token), use(expires), now;
            // clang-format on
        } else {
            int active = s.isActive() ? 1 : 0;
            int id = s.getId();
            // clang-format off
            ses << "UPDATE sessions SET is_active = ? WHERE id = ?",
                use(active), use(id), now;
            // clang-format on
        }
    } catch (const Poco::Exception& e) {
        throw DbException("SessionRepository::save: " + e.message());
    }
}

void SessionRepository::remove(int id) {
    try {
        auto ses = DbManager::instance().session();
        ses << "DELETE FROM sessions WHERE id = ?", use(id), now;
    } catch (const Poco::Exception& e) {
        throw DbException("SessionRepository::remove: " + e.message());
    }
}

std::optional<::Session> SessionRepository::findByToken(const std::string& token) {
    try {
        auto ses = DbManager::instance().session();
        int sid, userId, isActive;
        std::string tok, createdAt, expiresAt;
        // clang-format off
        std::string tokenParam = token;
        ses << "SELECT id, user_id, session_token, created_at, expires_at, is_active "
               "FROM sessions WHERE session_token = ? AND is_active = TRUE",
            into(sid), into(userId), into(tok),
            into(createdAt), into(expiresAt), into(isActive),
            use(tokenParam), now;
        // clang-format on
        if (tok.empty()) return std::nullopt;
        return rowToSession(sid, userId, tok, createdAt, expiresAt, isActive != 0);
    } catch (const Poco::Exception& e) {
        throw DbException("SessionRepository::findByToken: " + e.message());
    }
}

void SessionRepository::deactivateAllForUser(int userId) {
    try {
        auto ses = DbManager::instance().session();
        ses << "UPDATE sessions SET is_active = FALSE WHERE user_id = ?", use(userId), now;
    } catch (const Poco::Exception& e) {
        throw DbException("SessionRepository::deactivateAllForUser: " + e.message());
    }
}
