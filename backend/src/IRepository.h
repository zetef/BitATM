#pragma once
#include <optional>
#include <vector>

/**
 * @brief Abstract CRUD interface for a persistent store of domain objects.
 *
 * @tparam T Domain class that maps to a database table (must inherit IEntity).
 *
 * Concrete repositories (UserRepository, MessageRepository, etc.) inherit this
 * and implement each method via Poco::Data::Session queries.
 */
template <typename T>
class IRepository {
public:
    virtual ~IRepository() = default;

    /** @brief Find a single record by primary key. Returns nullopt if not found. */
    virtual std::optional<T> findById(int id) = 0;

    /** @brief Return all records in the table. */
    virtual std::vector<T> findAll() = 0;

    /** @brief Insert or update a record. */
    virtual void save(const T& entity) = 0;

    /** @brief Delete the record with the given primary key. */
    virtual void remove(int id) = 0;
};
