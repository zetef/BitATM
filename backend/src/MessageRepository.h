#pragma once
#include <optional>
#include <string>
#include <vector>

#include "DbManager.h"
#include "IRepository.h"
#include "Message.h"

/**
 * @brief PostgreSQL-backed repository for Message records.
 *
 * Inherits CRUD from IRepository<Message> and adds directional lookups.
 * All queries go through DbManager::instance().session().
 */
class MessageRepository : public IRepository<Message> {
public:
    MessageRepository() = default;
    ~MessageRepository() = default;

    std::optional<Message> findById(int id) override;
    std::vector<Message> findAll() override;
    void save(const Message& msg) override;
    void remove(int id) override;

    /** @brief Find all messages sent to a given recipient. */
    std::vector<Message> findByRecipient(const std::string& recipient);

    /** @brief Find all messages sent by a given sender. */
    std::vector<Message> findBySender(const std::string& sender);
};
