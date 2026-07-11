#pragma once
#include "IEntity.h"

/**
 * @brief Abstract middle layer for DB-mapped entities: owns the primary key.
 *
 * Implements getId() once for every entity. serialize(), deserialize() and
 * operator== stay pure from IEntity, so the class is abstract and can only
 * be instantiated through its subclasses. Copy/move special members are
 * protected to prevent slicing through a base reference.
 */
class BasePersistableEntity : public IEntity {
protected:
    int _id{0};

    BasePersistableEntity() = default;
    explicit BasePersistableEntity(int id) : _id(id) {}
    BasePersistableEntity(const BasePersistableEntity&) = default;
    BasePersistableEntity(BasePersistableEntity&&) noexcept = default;
    BasePersistableEntity& operator=(const BasePersistableEntity&) = default;
    BasePersistableEntity& operator=(BasePersistableEntity&&) noexcept = default;

public:
    ~BasePersistableEntity() override = default;

    /** @brief Primary key of the mapped row (0 when not yet persisted). */
    int getId() const override { return _id; }
};
