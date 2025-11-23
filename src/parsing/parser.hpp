#pragma once

#include <variant>
#include <cassert>
#include <complex>

#include "common/arena_allocator.hpp"
#include "ast/nodes.hpp"

class Parser {
public:
	explicit Parser(std::vector<Token> tokens);

	std::optional<NodeProgram> parse_prog();

private:
	std::optional<NodeAtom*> parse_atom();
	std::optional<VarType> parse_base_type();
	std::optional<NodeFuncCallExpr*> parse_expr_func_call();
	std::optional<NodeExpr*> parse_expr(const int minimum_precedence = 0);
	std::optional<NodeScope*> parse_scope();
	std::optional<NodeIfPredicate*> parse_if_predicate();
	std::optional<NodeStmt*> parse_stmt();

	[[nodiscard]] std::optional<Token> peek(const int offset = 0) const;
	Token consume();
	Token try_consume(const TokenType type, const std::string& err_msg);
	std::optional<Token> try_consume(const TokenType type);

	const std::vector<Token> m_tokens;
	size_t m_index = 0;
	ArenaAllocator m_allocator;
};