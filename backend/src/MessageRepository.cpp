#include "MessageRepository.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include "../../common/AppException.h"

using namespace Poco::Data::Keywords;

namespace {
Message rowToMessage(int id, const std::string& sender, const std::string& recipient,
                     const std::string& body, const std::string& key, const std::string& status,
                     const std::string& createdAt) {
    return Message{id, sender, recipient, body, key, status, createdAt};
}
}  // namespace

std::optional<Message> MessageRepository::findById(int id) {
    try {
        auto ses = DbManager::instance().session();
        int mid;
        std::string sender, recipient, body, key, status, createdAt;
        // clang-format off
        ses << "SELECT id, sender, recipient, body, encrypted_key, status, created_at "
               "FROM messages WHERE id = ?",
            into(mid), into(sender), into(recipient), into(body),
            into(key), into(status), into(createdAt),
            use(id), now;
        // clang-format on
        if (sender.empty()) return std::nullopt;
        return rowToMessage(mid, sender, recipient, body, key, status, createdAt);
    } catch (const Poco::Exception& e) {
        throw DbException("MessageRepository::findById: " + e.message());
    }
}

std::vector<Message> MessageRepository::findAll() {
    try {
        auto ses = DbManager::instance().session();
        std::vector<Message> msgs;
        int mid;
        std::string sender, recipient, body, key, status, createdAt;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, sender, recipient, body, encrypted_key, status, created_at FROM messages",
            into(mid), into(sender), into(recipient), into(body),
            into(key), into(status), into(createdAt), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!sender.empty())
                msgs.push_back(rowToMessage(mid, sender, recipient, body, key, status, createdAt));
        }
        return msgs;
    } catch (const Poco::Exception& e) {
        throw DbException("MessageRepository::findAll: " + e.message());
    }
}

void MessageRepository::save(const Message& msg) {
    try {
        auto ses = DbManager::instance().session();
        if (msg.getId() == 0) {
            // clang-format off
            ses << "INSERT INTO messages (sender, recipient, body, encrypted_key, status) "
                   "VALUES (?, ?, ?, ?, ?)",
                use(msg.getSender()), use(msg.getRecipient()),
                use(msg.getEncryptedKey()), use(msg.getStatus()), now;
            // clang-format on
        } else {
            // clang-format off
            ses << "UPDATE messages SET status = ? WHERE id = ?",
                use(msg.getStatus()), use(msg.getId()), now;
            // clang-format on
        }
    } catch (const Poco::Exception& e) {
        throw DbException("MessageRepository::save: " + e.message());
    }
}

void MessageRepository::remove(int id) {
    try {
        auto ses = DbManager::instance().session();
        ses << "DELETE FROM messages WHERE id = ?", use(id), now;
    } catch (const Poco::Exception& e) {
        throw DbException("MessageRepository::remove: " + e.message());
    }
}

std::vector<Message> MessageRepository::findByRecipient(const std::string& recipient) {
    try {
        auto ses = DbManager::instance().session();
        std::vector<Message> msgs;
        int mid;
        std::string sender, recip, body, key, status, createdAt;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, sender, recipient, body, encrypted_key, status, created_at "
               "FROM messages WHERE recipient = ? ORDER BY created_at ASC",
            into(mid), into(sender), into(recip), into(body),
            into(key), into(status), into(createdAt),
            use(recipient), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!sender.empty())
                msgs.push_back(rowToMessage(mid, sender, recip, body, key, status, createdAt));
        }
        return msgs;
    } catch (const Poco::Exception& e) {
        throw DbException("MessageRepository::findByRecipient: " + e.message());
    }
}

std::vector<Message> MessageRepository::findBySender(const std::string& sender) {
    try {
        auto ses = DbManager::instance().session();
        std::vector<Message> msgs;
        int mid;
        std::string sndr, recipient, body, key, status, createdAt;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, sender, recipient, body, encrypted_key, status, created_at "
               "FROM messages WHERE sender = ? ORDER BY created_at ASC",
            into(mid), into(sndr), into(recipient), into(body),
            into(key), into(status), into(createdAt),
            use(sender), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!sndr.empty())
                msgs.push_back(rowToMessage(mid, sndr, recipient, body, key, status, createdAt));
        }
        return msgs;
    } catch (const Poco::Exception& e) {
        throw DbException("MessageRepository::findBySender: " + e.message());
    }
}
