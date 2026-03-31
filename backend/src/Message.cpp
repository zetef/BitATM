#include "Message.h"

#include <sstream>

Message::Message(int id, std::string sender, std::string recipient, std::string encryptedBody,
                 std::string encryptedKey, std::string status, std::string createdAt)
    : _id(id),
      _sender(std::move(sender)),
      _recipient(std::move(recipient)),
      _encryptedBody(std::move(encryptedBody)),
      _encryptedKey(std::move(encryptedKey)),
      _status(std::move(status)),
      _createdAt(std::move(createdAt)) {}

std::string Message::serialize() const {
    // encryptedBody intentionally omitted — opaque to server
    return std::to_string(_id) + "|" + _sender + "|" + _recipient + "|" + _encryptedKey + "|" +
           _status + "|" + _createdAt;
}

void Message::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string token;
    std::getline(ss, token, '|');
    _id = std::stoi(token);
    std::getline(ss, _sender, '|');
    std::getline(ss, _recipient, '|');
    std::getline(ss, _encryptedKey, '|');
    std::getline(ss, _status, '|');
    std::getline(ss, _createdAt);
}

bool Message::operator==(const IEntity& other) const {
    const auto* m = dynamic_cast<const Message*>(&other);
    return m && _id == m->_id;
}

bool Message::operator<(const Message& other) const { return _createdAt < other._createdAt; }

bool Message::operator>(const Message& other) const { return _createdAt > other._createdAt; }

std::ostream& operator<<(std::ostream& os, const Message& m) {
    return os << "Message{id=" << m._id << ", from=" << m._sender << ", to=" << m._recipient
              << ", status=" << m._status << ", ts=" << m._createdAt << "}";
}
