#pragma once
#include <Poco/Data/SessionPool.h>

#include <memory>
#include <string>

/**
 * @brief Singleton that owns the PostgreSQL connection pool.
 *
 * Pattern: Meyers Singleton - thread-safe by C++11 static initialization.
 * Usage: DbManager::instance().session()
 *
 * Throws DbException if the pool is not initialized or a connection fails.
 */
class DbManager {
public:
    /** @brief Access the singleton instance. */
    static DbManager& instance();

    /**
     * @brief Initialize the connection pool.
     * Call once at startup before any repository is used.
     * @param connectionString e.g. "host=localhost dbname=bitatm user=chatuser password=changeme"
     * @param poolSize         Number of pooled connections (default 4).
     */
    void init(const std::string& connectionString, int poolSize = 4);

    /**
     * @brief Borrow a session from the pool.
     * Returns a session to the pool automatically when it goes out of scope.
     */
    Poco::Data::Session session();

    // Singleton - non-copyable, non-movable
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;
    DbManager(DbManager&&) = delete;
    DbManager& operator=(DbManager&&) = delete;

private:
    DbManager() = default;
    ~DbManager() = default;

    std::unique_ptr<Poco::Data::SessionPool> _pool;
};
