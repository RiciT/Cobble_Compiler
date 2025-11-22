#pragma once

#include "lexing/token.hpp"

inline std::optional<int> bin_precedence(const TokenType type) {
    switch(type) {
        case TokenType::equals_equals:
        case TokenType::not_equals:
        case TokenType::greater_equals:
        case TokenType::less_equals:
        case TokenType::greater:
        case TokenType::less:
            return 0;
        case TokenType::plus_sign:
        case TokenType::dash_sign:
            return 1;
        case TokenType::star_sign:
        case TokenType::fslash_sign:
            return 2;
        default:
            return {};
    }
}
