#include "UserRepository.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include "../../common/AppException.h"

using namespace Poco::Data::Keywords;

std::optional<User> UserRepository::findById(int id) {
    try {
        auto ses = DbManager::instance().session();
        int uid;
        std::string username, passwordHash, publicKey, lastSeen, createdAt;
        // clang-format off
        ses << "SELECT id, username, password_hash, public_key, last_seen, created_at "
               "FROM users WHERE id = ?",
            into(uid), into(username), into(passwordHash),
            into(publicKey), into(lastSeen), into(createdAt),
            use(id), now;
        // clang-format on
        if (username.empty()) return std::nullopt;
        return User{uid, username, passwordHash, publicKey, lastSeen, createdAt};
    } catch (const Poco::Exception& e) {
        throw DbException("UserRepository::findById: " + e.message());
    }
}

std::vector<User> UserRepository::findAll() {
    try {
        auto ses = DbManager::instance().session();
        std::vector<User> users;
        int uid;
        std::string username, passwordHash, publicKey, lastSeen, createdAt;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, username, password_hash, public_key, last_seen, created_at FROM users",
            into(uid), into(username), into(passwordHash),
            into(publicKey), into(lastSeen), into(createdAt), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!username.empty())
                users.emplace_back(uid, username, passwordHash, publicKey, lastSeen, createdAt);
        }
        return users;
    } catch (const Poco::Exception& e) {
        throw DbException("UserRepository::findAll: " + e.message());
    }
}

void UserRepository::save(const User& u) {
    try {
        auto ses = DbManager::instance().session();
        if (u.getId() == 0) {
            std::string username = u.getUsername();
            std::string passwordHash = u.getPasswordHash();
            std::string publicKey = u.getPublicKey();
            // clang-format off
            ses << "INSERT INTO users (username, password_hash, public_key) VALUES (?, ?, ?)",
                use(username), use(passwordHash), use(publicKey), now;
            // clang-format on
        } else {
            std::string publicKey = u.getPublicKey();
            int id = u.getId();
            // clang-format off
            ses << "UPDATE users SET public_key = ?, last_seen = NOW() WHERE id = ?",
                use(publicKey), use(id), now;
            // clang-format on
        }
    } catch (const Poco::Exception& e) {
        throw DbException("UserRepository::save: " + e.message());
    }
}

void UserRepository::remove(int id) {
    try {
        auto ses = DbManager::instance().session();
        ses << "DELETE FROM users WHERE id = ?", use(id), now;
    } catch (const Poco::Exception& e) {
        throw DbException("UserRepository::remove: " + e.message());
    }
}

std::optional<User> UserRepository::findByUsername(const std::string& username) {
    try {
        auto ses = DbManager::instance().session();
        int uid;
        std::string uname, passwordHash, publicKey, lastSeen, createdAt;
        // clang-format off
        std::string usernameParam = username;
        ses << "SELECT id, username, password_hash, public_key, last_seen, created_at "
               "FROM users WHERE username = ?",
            into(uid), into(uname), into(passwordHash),
            into(publicKey), into(lastSeen), into(createdAt),
            use(usernameParam), now;
        // clang-format on
        if (uname.empty()) return std::nullopt;
        return User{uid, uname, passwordHash, publicKey, lastSeen, createdAt};
    } catch (const Poco::Exception& e) {
        throw DbException("UserRepository::findByUsername: " + e.message());
    }
}
