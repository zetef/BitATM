#include "GroupMessage.h"

#include <sstream>

GroupMessage::GroupMessage(int id, std::string groupName, std::string sender,
                           std::string encryptedBody, std::string encryptedKeys,
                           std::string createdAt)
    : _id(id),
      _groupName(std::move(groupName)),
      _sender(std::move(sender)),
      _encryptedBody(std::move(encryptedBody)),
      _encryptedKeys(std::move(encryptedKeys)),
      _createdAt(std::move(createdAt)) {}

std::string GroupMessage::serialize() const {
    // encryptedBody intentionally omitted — opaque to server
    return std::to_string(_id) + "|" + _groupName + "|" + _sender + "|" + _encryptedKeys + "|" +
           _createdAt;
}

void GroupMessage::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string token;
    std::getline(ss, token, '|');
    _id = std::stoi(token);
    std::getline(ss, _groupName, '|');
    std::getline(ss, _sender, '|');
    std::getline(ss, _encryptedKeys, '|');
    std::getline(ss, _createdAt);
}

bool GroupMessage::operator==(const IEntity& other) const {
    const auto* g = dynamic_cast<const GroupMessage*>(&other);
    return g && _id == g->_id;
}
