#pragma once

#include <string_view>
#include "common/unordered_bimap.hpp"

enum class TokenType
{
    //Atoms
    ident,
    int_lit,
    //Keywords
    exit_,
    def_,
    func_,
    if_,
    elseif_,
    else_,
    while_,
    print_,
    return_,
    true_,
    false_,
    //types
    int_,
    char_,
    bool_,
    //Boolean operators
    equals_equals,
    not_equals,
    greater_equals,
    less_equals,
    greater,
    less,
    //Single char tokens
    semi,
    open_paren,
    close_paren,
    equals,
    plus_sign,
    star_sign,
    dash_sign,
    fslash_sign,
    open_curly,
    close_curly,
    comma,
    single_quote,
    double_quote,
    exclamation_point,
    open_bracket,
    close_bracket
};

static const bidirectional_unordered_map<std::string_view, TokenType> KeyWordTokens = {
    {"exit", TokenType::exit_},
    {"def", TokenType::def_},
    {"func", TokenType::func_},
    {"if", TokenType::if_},
    {"elseif", TokenType::elseif_},
    {"else", TokenType::else_},
    {"while", TokenType::while_},
    {"print", TokenType::print_},
    {"return", TokenType::return_},
    {"int", TokenType::int_},
    {"char", TokenType::char_},
    {"bool", TokenType::bool_},
    {"true", TokenType::true_},
    {"false", TokenType::false_},
};

static const bidirectional_unordered_map<char, TokenType> SingleCharTokens = {
    {'(', TokenType::open_paren},
    {')', TokenType::close_paren},
    {'+', TokenType::plus_sign},
    {'-', TokenType::dash_sign},
    {'*', TokenType::star_sign},
    {'/', TokenType::fslash_sign},
    {'=', TokenType::equals},
    {';', TokenType::semi},
    {'{', TokenType::open_curly},
    {'}', TokenType::close_curly},
    {',', TokenType::comma},
    {'\'', TokenType::single_quote},
    {'\"', TokenType::double_quote},
    {'>', TokenType::greater},
    {'<', TokenType::less},
    {'!', TokenType::exclamation_point},
    {'[', TokenType::open_bracket},
    {']', TokenType::close_bracket},
};

struct Token
{
    TokenType type;
    std::optional<std::string_view> value {};
    size_t line = 1; //default to line 1
};
