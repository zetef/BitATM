#pragma once
#include <optional>
#include <string>
#include <vector>

#include "BaseSqlRepository.h"
#include "OfflineMessage.h"

/**
 * @brief PostgreSQL-backed repository for the offline_queue table.
 *
 * Inherits CRUD from IRepository<OfflineMessage> and adds delivery helpers.
 * Used by the server to buffer messages for recipients who are offline.
 */
class OfflineQueueRepository : public BaseSqlRepository<OfflineMessage> {
public:
    /** @brief Delivery tries per entry before it is skipped and later purged. */
    static constexpr int MAX_DELIVERY_ATTEMPTS = 5;

    OfflineQueueRepository() : BaseSqlRepository("offline_queue") {}
    ~OfflineQueueRepository() = default;

    std::optional<OfflineMessage> findById(int id) override;
    std::vector<OfflineMessage> findAll() override;
    void save(const OfflineMessage& msg) override;

    /**
     * @brief Return undelivered entries for a recipient (called on reconnect).
     *
     * Entries that reached MAX_DELIVERY_ATTEMPTS are excluded; they await
     * purge by cleanupExhausted().
     */
    std::vector<OfflineMessage> findUndeliveredByRecipient(const std::string& recipient);

    /** @brief Mark an entry as delivered. */
    void markDelivered(int id);

    /** @brief Increment the delivery attempt counter for an entry. */
    void incrementAttempts(int id);

    /** @brief Delete rows that have been delivered and are older than 7 days. */
    void cleanupDelivered();

    /** @brief Delete undelivered rows that reached MAX_DELIVERY_ATTEMPTS. */
    void cleanupExhausted();
};
