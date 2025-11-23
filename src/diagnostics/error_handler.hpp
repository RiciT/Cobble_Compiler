#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "lexing/token.hpp"

struct Error {
    std::string msg;
    size_t line;
};

class ErrorHandler {
public:
    void report(const std::string& message, const size_t line) {
        m_errors.push_back({ .msg = message, .line = line });
    }

    void report(const std::string& message, const Token& token) {
        report(message, token.line);
    }

    [[nodiscard]] bool has_errors() const {
        return !m_errors.empty();
    }

    void dump_and_exit() const {
        if (m_errors.empty()) return;

        for (const auto&[msg, line] : m_errors) {
            std::cerr << "[Line " << line << "] Error: " << msg << std::endl;
        }
        exit(EXIT_FAILURE);
    }

private:
    std::vector<Error> m_errors;

};