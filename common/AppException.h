#pragma once
#include <stdexcept>
#include <string>

class AppException : public std::runtime_error {
public:
    explicit AppException(const std::string& msg, int code = 0)
        : std::runtime_error(msg), _code(code) {}
    int code() const { return _code; }

private:
    int _code;
};

class CryptoException : public AppException {
public:
    using AppException::AppException;
};

class NetworkException : public AppException {
public:
    using AppException::AppException;
};

class DbException : public AppException {
public:
    using AppException::AppException;
};

class ProtocolException : public AppException {
public:
    using AppException::AppException;
};