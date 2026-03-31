#pragma once
#include <ostream>
#include <string>

#include "IEntity.h"

/**
 * @brief A chat message. Maps to the `messages` table.
 *
 * operator<< never prints the encrypted body.
 * operator< and operator> order by created_at timestamp.
 * operator== compares by id.
 */
class Message : public IEntity {
    int _id{0};
    std::string _sender;
    std::string _recipient;
    std::string _encryptedBody;
    std::string _encryptedKey;
    std::string _status;
    std::string _createdAt;

public:
    Message() = default;
    Message(int id, std::string sender, std::string recipient, std::string encryptedBody,
            std::string encryptedKey, std::string status = "sent", std::string createdAt = {});
    Message(const Message&) = default;
    Message(Message&&) noexcept = default;
    Message& operator=(const Message&) = default;
    Message& operator=(Message&&) noexcept = default;
    ~Message() override = default;

    int getId() const override { return _id; }
    const std::string& getSender() const { return _sender; }
    const std::string& getRecipient() const { return _recipient; }
    const std::string& getEncryptedKey() const { return _encryptedKey; }
    const std::string& getStatus() const { return _status; }
    const std::string& getCreatedAt() const { return _createdAt; }

    void setStatus(const std::string& s) { _status = s; }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
    bool operator==(const IEntity& other) const override;
    bool operator<(const Message& other) const;
    bool operator>(const Message& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Message& m);
};
