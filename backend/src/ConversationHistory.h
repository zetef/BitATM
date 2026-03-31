#pragma once
#include <string>
#include <vector>

#include "IEntity.h"
#include "Message.h"

/**
 * @brief Ordered message history between two users.
 *
 * operator[] accesses by position.
 * operator+  merges two histories, deduplicates by id, and sorts by timestamp.
 * operator== compares by the two participant usernames (order-insensitive).
 */
class ConversationHistory : public IEntity {
    int _id{0};
    std::string _user1;
    std::string _user2;
    std::vector<Message> _messages;

public:
    ConversationHistory() = default;
    ConversationHistory(int id, std::string user1, std::string user2,
                        std::vector<Message> messages = {});
    ConversationHistory(const ConversationHistory&) = default;
    ConversationHistory(ConversationHistory&&) noexcept = default;
    ConversationHistory& operator=(const ConversationHistory&) = default;
    ConversationHistory& operator=(ConversationHistory&&) noexcept = default;
    ~ConversationHistory() override = default;

    int getId() const override { return _id; }
    const std::string& getUser1() const { return _user1; }
    const std::string& getUser2() const { return _user2; }
    const std::vector<Message>& getMessages() const { return _messages; }
    std::size_t size() const { return _messages.size(); }

    void append(Message msg) { _messages.push_back(std::move(msg)); }

    /** @brief Access message by position. */
    const Message& operator[](std::size_t index) const;

    /** @brief Merge two histories: deduplicate by id, sort by timestamp. */
    ConversationHistory operator+(const ConversationHistory& other) const;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
    bool operator==(const IEntity& other) const override;
};
