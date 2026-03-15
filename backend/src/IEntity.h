#pragma once
#include <string>

/**
 * @brief Base interface for all persistent domain objects.
 *
 * Every class that maps to a database row inherits this.
 */
class IEntity {
public:
    virtual ~IEntity() = default;

    virtual int getId() const = 0;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;
    virtual bool operator==(const IEntity& other) const = 0;
};