#pragma once
#include <optional>
#include <string>
#include <vector>

#include "BaseSqlRepository.h"
#include "Message.h"

/**
 * @brief PostgreSQL-backed repository for Message records.
 *
 * Inherits CRUD via BaseSqlRepository<Message> (shared session plumbing, generic remove()) and adds
 * directional lookups. All queries go through DbManager::instance().session().
 */
class MessageRepository : public BaseSqlRepository<Message> {
public:
    MessageRepository() : BaseSqlRepository("messages") {}
    ~MessageRepository() = default;

    std::optional<Message> findById(int id) override;
    std::vector<Message> findAll() override;
    void save(const Message& msg) override;

    /** @brief Find all messages sent to a given recipient. */
    std::vector<Message> findByRecipient(const std::string& recipient);

    /** @brief Find all messages sent by a given sender. */
    std::vector<Message> findBySender(const std::string& sender);

    /**
     * @brief Find all messages where user is sender or recipient,
     *        with created_at strictly after the given ISO 8601 cursor.
     *        If cursor is empty, returns all messages for the user.
     */
    std::vector<Message> findAllForUser(const std::string& username,
                                        const std::string& afterTimestamp = "");
};
