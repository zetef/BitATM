#pragma once
#include <string>
#include <vector>

#include "../../common/protocol.h"

/**
 * @brief One queued packet a user should receive the next time they log in.
 */
struct PendingNotification {
    int id = 0;
    PacketType type;
    std::string fromUser;
    std::string body;
};

/**
 * @brief PostgreSQL-backed repository for the pending_notifications table.
 *
 * Generic store-and-forward for control-plane packets (e.g. DELETE_CONVERSATION,
 * GROUP_LEAVE) aimed at a user who is offline. Distinct from
 * OfflineQueueRepository, which carries E2EE message ciphertext tied to a
 * messages.id row - this table only carries small notification packets.
 *
 * Not an IRepository<T> - like GroupRepository, it does not map to a single
 * IEntity subclass.
 */
class PendingNotificationRepository {
public:
    PendingNotificationRepository() = default;

    /**
     * @brief Queue a packet for delivery the next time recipient logs in.
     * @param recipient Username who should receive the packet.
     * @param type Packet type to reconstruct on flush.
     * @param fromUser Value for the reconstructed packet's `from` field.
     * @param body Value for the reconstructed packet's `body` field (e.g. group id).
     */
    void queue(const std::string& recipient, PacketType type, const std::string& fromUser,
               const std::string& body = "");

    /**
     * @brief Return all pending notifications for a recipient and delete them.
     *
     * Consume-once semantics: a second call for the same recipient with no
     * new activity returns an empty vector.
     */
    std::vector<PendingNotification> findAndClear(const std::string& recipient);
};
