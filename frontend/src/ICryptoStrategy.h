#pragma once
#include <QByteArray>
#include <QString>

/**
 * @brief Strategy interface for encryption algorithms.
 *
 * Allows swapping AES-GCM for any future algorithm
 * without changing calling code.
 */
class ICryptoStrategy {
public:
    virtual ~ICryptoStrategy() = default;

    virtual QByteArray encrypt(const QByteArray& data, const QByteArray& key) = 0;
    virtual QByteArray decrypt(const QByteArray& data, const QByteArray& key) = 0;
    virtual QString name() const = 0;
};