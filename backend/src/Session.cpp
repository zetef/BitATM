#include "Session.h"

#include <sstream>

Session::Session(int id, int userId, std::string sessionToken, std::string createdAt,
                 std::string expiresAt, bool isActive)
    : _id(id),
      _userId(userId),
      _sessionToken(std::move(sessionToken)),
      _createdAt(std::move(createdAt)),
      _expiresAt(std::move(expiresAt)),
      _isActive(isActive) {}

std::string Session::serialize() const {
    return std::to_string(_id) + "|" + std::to_string(_userId) + "|" + _sessionToken + "|" +
           _createdAt + "|" + _expiresAt + "|" + (_isActive ? "1" : "0");
}

void Session::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string token;
    std::getline(ss, token, '|');
    _id = std::stoi(token);
    std::getline(ss, token, '|');
    _userId = std::stoi(token);
    std::getline(ss, _sessionToken, '|');
    std::getline(ss, _createdAt, '|');
    std::getline(ss, _expiresAt, '|');
    std::getline(ss, token);
    _isActive = (token == "1");
}

bool Session::operator==(const IEntity& other) const {
    const auto* s = dynamic_cast<const Session*>(&other);
    return s && _sessionToken == s->_sessionToken;
}
