#include "OfflineMessage.h"

#include <sstream>

OfflineMessage::OfflineMessage(int id, int messageId, std::string recipient, std::string queuedAt,
                               bool delivered, int deliveryAttempts)
    : _id(id),
      _messageId(messageId),
      _recipient(std::move(recipient)),
      _queuedAt(std::move(queuedAt)),
      _delivered(delivered),
      _deliveryAttempts(deliveryAttempts) {}

std::string OfflineMessage::serialize() const {
    return std::to_string(_id) + "|" + std::to_string(_messageId) + "|" + _recipient + "|" +
           _queuedAt + "|" + (_delivered ? "1" : "0") + "|" + std::to_string(_deliveryAttempts);
}

void OfflineMessage::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string token;
    std::getline(ss, token, '|');
    _id = std::stoi(token);
    std::getline(ss, token, '|');
    _messageId = std::stoi(token);
    std::getline(ss, _recipient, '|');
    std::getline(ss, _queuedAt, '|');
    std::getline(ss, token, '|');
    _delivered = (token == "1");
    std::getline(ss, token);
    _deliveryAttempts = std::stoi(token);
}

bool OfflineMessage::operator==(const IEntity& other) const {
    const auto* o = dynamic_cast<const OfflineMessage*>(&other);
    return o && _id == o->_id;
}
