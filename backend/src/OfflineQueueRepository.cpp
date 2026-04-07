#include "OfflineQueueRepository.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include "../../common/AppException.h"

using namespace Poco::Data::Keywords;

namespace {
OfflineMessage rowToOfflineMessage(int id, int messageId, const std::string& recipient,
                                   const std::string& queuedAt, int delivered,
                                   int deliveryAttempts) {
    return OfflineMessage{id, messageId, recipient, queuedAt, delivered != 0, deliveryAttempts};
}
}  // namespace

std::optional<OfflineMessage> OfflineQueueRepository::findById(int id) {
    try {
        auto ses = DbManager::instance().session();
        int oid, messageId, delivered, deliveryAttempts;
        std::string recipient, queuedAt;
        // clang-format off
        ses << "SELECT id, message_id, recipient, queued_at, delivered, delivery_attempts "
               "FROM offline_queue WHERE id = ?",
            into(oid), into(messageId), into(recipient),
            into(queuedAt), into(delivered), into(deliveryAttempts),
            use(id), now;
        // clang-format on
        if (recipient.empty()) return std::nullopt;
        return rowToOfflineMessage(oid, messageId, recipient, queuedAt, delivered,
                                   deliveryAttempts);
    } catch (const Poco::Exception& e) {
        throw DbException("OfflineQueueRepository::findById: " + e.message());
    }
}

std::vector<OfflineMessage> OfflineQueueRepository::findAll() {
    try {
        auto ses = DbManager::instance().session();
        std::vector<OfflineMessage> msgs;
        int oid, messageId, delivered, deliveryAttempts;
        std::string recipient, queuedAt;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, message_id, recipient, queued_at, delivered, delivery_attempts "
               "FROM offline_queue",
            into(oid), into(messageId), into(recipient),
            into(queuedAt), into(delivered), into(deliveryAttempts), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!recipient.empty())
                msgs.push_back(rowToOfflineMessage(oid, messageId, recipient, queuedAt, delivered,
                                                   deliveryAttempts));
        }
        return msgs;
    } catch (const Poco::Exception& e) {
        throw DbException("OfflineQueueRepository::findAll: " + e.message());
    }
}

void OfflineQueueRepository::save(const OfflineMessage& msg) {
    try {
        auto ses = DbManager::instance().session();
        if (msg.getId() == 0) {
            // clang-format off
            ses << "INSERT INTO offline_queue (message_id, recipient) VALUES (?, ?)",
                use(msg.getMessageId()), use(msg.getRecipient()), now;
            // clang-format on
        } else {
            int delivered = msg.isDelivered() ? 1 : 0;
            int attempts = msg.getDeliveryAttempts();
            // clang-format off
            ses << "UPDATE offline_queue SET delivered = ?, delivery_attempts = ? WHERE id = ?",
                use(delivered), use(attempts), use(msg.getId()), now;
            // clang-format on
        }
    } catch (const Poco::Exception& e) {
        throw DbException("OfflineQueueRepository::save: " + e.message());
    }
}

void OfflineQueueRepository::remove(int id) {
    try {
        auto ses = DbManager::instance().session();
        ses << "DELETE FROM offline_queue WHERE id = ?", use(id), now;
    } catch (const Poco::Exception& e) {
        throw DbException("OfflineQueueRepository::remove: " + e.message());
    }
}

std::vector<OfflineMessage> OfflineQueueRepository::findUndeliveredByRecipient(
    const std::string& recipient) {
    try {
        auto ses = DbManager::instance().session();
        std::vector<OfflineMessage> msgs;
        int oid, messageId, delivered, deliveryAttempts;
        std::string recip, queuedAt;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, message_id, recipient, queued_at, delivered, delivery_attempts "
               "FROM offline_queue WHERE recipient = ? AND delivered = FALSE ORDER BY queued_at ASC",
            into(oid), into(messageId), into(recip),
            into(queuedAt), into(delivered), into(deliveryAttempts),
            use(recipient), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (!recip.empty())
                msgs.push_back(rowToOfflineMessage(oid, messageId, recip, queuedAt, delivered,
                                                   deliveryAttempts));
        }
        return msgs;
    } catch (const Poco::Exception& e) {
        throw DbException("OfflineQueueRepository::findUndeliveredByRecipient: " + e.message());
    }
}

void OfflineQueueRepository::markDelivered(int id) {
    try {
        auto ses = DbManager::instance().session();
        ses << "UPDATE offline_queue SET delivered = TRUE WHERE id = ?", use(id), now;
    } catch (const Poco::Exception& e) {
        throw DbException("OfflineQueueRepository::markDelivered: " + e.message());
    }
}
