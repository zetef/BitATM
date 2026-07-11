#pragma once
#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include <string>

#include "../../common/AppException.h"
#include "DbManager.h"
#include "IRepository.h"

/**
 * @brief Abstract middle layer for PostgreSQL-backed repositories.
 *
 * Owns the mapped table name and the shared Poco::Data plumbing, and
 * implements remove() once - every mapped table has id SERIAL PRIMARY KEY.
 * findById(), findAll() and save() stay pure from IRepository, so the
 * class is abstract.
 *
 * @tparam T Entity type handled by the concrete repository.
 */
template <typename T>
class BaseSqlRepository : public IRepository<T> {
    std::string _table;

protected:
    /**
     * @brief Bind the repository to its SQL table.
     * @param table Table name; always a subclass string literal, never user input.
     */
    explicit BaseSqlRepository(std::string table) : _table(std::move(table)) {}

    /** @brief Borrow one pooled session from the DbManager singleton. */
    Poco::Data::Session session() { return DbManager::instance().session(); }

    /** @brief Name of the mapped table. */
    const std::string& table() const { return _table; }

public:
    ~BaseSqlRepository() override = default;

    /** @brief Delete the row with the given primary key. */
    void remove(int id) override {
        try {
            auto ses = session();
            ses << "DELETE FROM " + _table + " WHERE id = $1", Poco::Data::Keywords::use(id),
                Poco::Data::Keywords::now;
        } catch (const Poco::Exception& e) {
            throw DbException(_table + " remove: " + e.message());
        }
    }
};
