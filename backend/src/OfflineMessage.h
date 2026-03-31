#pragma once
#include <string>

#include "IEntity.h"

/**
 * @brief A queued message for an offline recipient. Maps to the `offline_queue` table.
 *
 * operator== compares by id.
 */
class OfflineMessage : public IEntity {
    int _id{0};
    int _messageId{0};
    std::string _recipient;
    std::string _queuedAt;
    bool _delivered{false};
    int _deliveryAttempts{0};

public:
    OfflineMessage() = default;
    OfflineMessage(int id, int messageId, std::string recipient, std::string queuedAt,
                   bool delivered = false, int deliveryAttempts = 0);
    OfflineMessage(const OfflineMessage&) = default;
    OfflineMessage(OfflineMessage&&) noexcept = default;
    OfflineMessage& operator=(const OfflineMessage&) = default;
    OfflineMessage& operator=(OfflineMessage&&) noexcept = default;
    ~OfflineMessage() override = default;

    int getId() const override { return _id; }
    int getMessageId() const { return _messageId; }
    const std::string& getRecipient() const { return _recipient; }
    const std::string& getQueuedAt() const { return _queuedAt; }
    bool isDelivered() const { return _delivered; }
    int getDeliveryAttempts() const { return _deliveryAttempts; }

    void markDelivered() { _delivered = true; }
    void incrementAttempts() { ++_deliveryAttempts; }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
    bool operator==(const IEntity& other) const override;
};
