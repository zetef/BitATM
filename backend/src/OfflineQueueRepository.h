#pragma once
#include <optional>
#include <string>
#include <vector>

#include "DbManager.h"
#include "IRepository.h"
#include "OfflineMessage.h"

/**
 * @brief PostgreSQL-backed repository for the offline_queue table.
 *
 * Inherits CRUD from IRepository<OfflineMessage> and adds delivery helpers.
 * Used by the server to buffer messages for recipients who are offline.
 */
class OfflineQueueRepository : public IRepository<OfflineMessage> {
public:
    OfflineQueueRepository() = default;
    ~OfflineQueueRepository() = default;

    std::optional<OfflineMessage> findById(int id) override;
    std::vector<OfflineMessage> findAll() override;
    void save(const OfflineMessage& msg) override;
    void remove(int id) override;

    /** @brief Return all undelivered entries for a recipient (called on reconnect). */
    std::vector<OfflineMessage> findUndeliveredByRecipient(const std::string& recipient);

    /** @brief Mark an entry as delivered and reset retry counter. */
    void markDelivered(int id);
};
