#include "ConversationHistory.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

ConversationHistory::ConversationHistory(int id, std::string user1, std::string user2,
                                         std::vector<Message> messages)
    : _id(id), _user1(std::move(user1)), _user2(std::move(user2)), _messages(std::move(messages)) {}

const Message& ConversationHistory::operator[](std::size_t index) const {
    if (index >= _messages.size())  // size_t is implicitly >= 0
        throw std::out_of_range("ConversationHistory: index out of range");
    return _messages[index];
}

ConversationHistory ConversationHistory::operator+(const ConversationHistory& other) const {
    std::vector<Message> merged = _messages;
    std::unordered_set<int> seen;
    for (const auto& m : _messages) seen.insert(m.getId());
    for (const auto& m : other._messages)
        if (seen.insert(m.getId()).second) merged.push_back(m);
    std::sort(merged.begin(), merged.end(),
              [](const Message& a, const Message& b) { return a < b; });
    return ConversationHistory{_id, _user1, _user2, std::move(merged)};
}

std::string ConversationHistory::serialize() const {
    return std::to_string(_id) + "|" + _user1 + "|" + _user2 + "|" +
           std::to_string(_messages.size());
}

void ConversationHistory::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string token;
    std::getline(ss, token, '|');
    _id = std::stoi(token);
    std::getline(ss, _user1, '|');
    std::getline(ss, _user2, '|');
    // messages are loaded separately by the repository
}

bool ConversationHistory::operator==(const IEntity& other) const {
    const auto* c = dynamic_cast<const ConversationHistory*>(&other);
    if (!c) return false;
    return (_user1 == c->_user1 && _user2 == c->_user2) ||
           (_user1 == c->_user2 && _user2 == c->_user1);
}
