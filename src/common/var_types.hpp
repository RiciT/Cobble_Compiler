#pragma once

#include "../lexing/token.hpp"

enum class BaseType {
    int_,
    char_,
    bool_,
};

struct VarType {
    BaseType base;
    bool is_array = false;
    size_t array_size = 0;

    //helpers
    //comparison

    bool operator==(const VarType& other) const
    {
        return base == other.base &&
            is_array == other.is_array &&
            array_size == other.array_size;
    }

    bool operator!=(const VarType& other) const
    {
        return !(*this == other);
    }

    //base match
    bool base_matches(const VarType& other) const
    {
        return base == other.base;
    }

    //get element type
    VarType element_type() const
    {
        return VarType{ .base = base, .is_array = false, .array_size = 0};
    }

    //create array type from this array
    VarType as_array(const size_t size) const
    {
        return VarType{ .base = base, .is_array = true, .array_size = size};
    }

    //helpers to create types easily
    static VarType make_int_lit()
    {
        return VarType{ .base = BaseType::int_, .is_array = false, .array_size = 0};
    }

    static VarType make_bool_lit()
    {
        return VarType{ .base = BaseType::bool_, .is_array = false, .array_size = 0};
    }

    static VarType make_int_array(const size_t size)
    {
        return VarType{ .base = BaseType::int_, .is_array = true, .array_size = size};
    }

    static VarType make_bool_array(const size_t size)
    {
        return VarType{ .base = BaseType::bool_, .is_array = true, .array_size = size};
    }
};

static const bidirectional_unordered_map<BaseType, TokenType> VariableBaseTypes = {
    {BaseType::int_, TokenType::int_},
    {BaseType::char_, TokenType::char_},
    {BaseType::bool_, TokenType::bool_},
};
