#include "User.h"

#include <algorithm>
#include <cctype>
#include <sstream>

User::User(int id, std::string username, std::string passwordHash, std::string publicKey,
           std::string lastSeen, std::string createdAt)
    : _id(id),
      _username(std::move(username)),
      _passwordHash(std::move(passwordHash)),
      _publicKey(std::move(publicKey)),
      _lastSeen(std::move(lastSeen)),
      _createdAt(std::move(createdAt)) {}

std::string User::serialize() const {
    // password hash intentionally omitted
    return std::to_string(_id) + "|" + _username + "|" + _publicKey + "|" + _lastSeen + "|" +
           _createdAt;
}

void User::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string token;
    std::getline(ss, token, '|');
    _id = std::stoi(token);
    std::getline(ss, _username, '|');
    std::getline(ss, _publicKey, '|');
    std::getline(ss, _lastSeen, '|');
    std::getline(ss, _createdAt);
}

bool User::operator==(const IEntity& other) const {
    const auto* u = dynamic_cast<const User*>(&other);
    if (!u) return false;
    std::string a = _username, b = u->_username;
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return std::tolower(c); });
    std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return std::tolower(c); });
    return a == b;
}

std::ostream& operator<<(std::ostream& os, const User& u) {
    return os << "User{id=" << u._id << ", username=" << u._username
              << ", publicKey=" << (u._publicKey.empty() ? "<none>" : "<set>")
              << ", lastSeen=" << u._lastSeen << "}";
}
