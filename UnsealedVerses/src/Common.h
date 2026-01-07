#pragma once
#include <vector>
#include <string>
#include <expected>
#include <stdexcept>

template<typename T, typename E>
inline T unwrap(std::expected<T, E>&& result, const std::string& error_context) {
    if (result) {
        return std::move(*result);
    }
    throw std::runtime_error(error_context + ": " + result.error().message);
}

template<typename E>
inline void unwrap(std::expected<void, E>&& result, const std::string& error_context) {
    if (!result) {
        throw std::runtime_error(error_context + ": " + result.error().message);
    }
}

class Command {
public:
    virtual ~Command() = default;
    virtual int execute() = 0;
protected:
    Command(std::vector<std::string> args) : m_args(std::move(args)) {}
    std::vector<std::string> m_args;
};