#pragma once

#include <variant>
#include <cassert>

#include "tokenization.hpp"
#include "arena_allocator.hpp"

struct NodeAtomIntLit {
	Token int_lit;
};

struct NodeAtomIdent {
	Token ident;
};

struct NodeExpr;

struct NodeBinExprAdd {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

// struct NodeBinExprMult {
// 	NodeExpr* lhs;
// 	NodeExpr* rhs;
// };

struct NodeBinExpr {
	NodeBinExprAdd* bin_expr;
};

struct NodeAtom {
	std::variant<NodeAtomIntLit*, NodeAtomIdent*> primary_expr;
};

struct NodeExpr {
	std::variant<NodeAtom*, NodeBinExpr*> expr;
};

struct NodeStmtExit {
	NodeExpr* expr;
};

struct NodeStmtLet {
	Token ident;
	NodeExpr* expr;
};

struct NodeStmt {
	std::variant<NodeStmtExit*, NodeStmtLet*> stmt;
};

struct NodeProgram {
	std::vector<NodeStmt*> stmts;
};

class Parser {
public:
	inline explicit Parser(std::vector<Token> tokens)
		: m_tokens(std::move(tokens)),
		m_allocator(1024 * 1024 * 4) //4 Mb
	{
	}

	std::optional<NodeBinExpr*> parse_bin_expr()
	{
		if (auto lhs = parse_expr())
		{
			auto bin_expr = m_allocator.alloc<NodeBinExpr>();
			if (peek().has_value() && peek().value().type == TokenType::plus_sign)
			{
				auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();
				bin_expr_add->lhs = lhs.value();
				
				consume();
				if (auto rhs = parse_expr())
				{
					bin_expr_add->rhs = rhs.value();
					bin_expr->bin_expr = bin_expr_add;
					return bin_expr;
				}
				else
				{
					std::cerr << "Expected expression" << std::endl;
					exit(EXIT_FAILURE);
				}
			}
			else 
			{
				std::cerr << "Unsupported binary operator" << std::endl;
				exit(EXIT_FAILURE);
			}
			
		}
		else
		{
			return {};
		}
		
	}

	std::optional<NodeAtom*> parse_atom() 
	{
		if (peek().has_value() && peek().value().type == TokenType::int_lit)			
		{
			auto atom_int_lit = m_allocator.alloc<NodeAtomIntLit>();
			atom_int_lit->int_lit = consume();
			auto atom = m_allocator.alloc<NodeAtom>();
			atom->primary_expr = atom_int_lit;
			return atom;
		}
		else if (peek().has_value() && peek().value().type == TokenType::ident) 
		{
			auto atom_ident = m_allocator.alloc<NodeAtomIdent>();
			atom_ident->ident = consume();
			auto atom = m_allocator.alloc<NodeAtom>();
			atom->primary_expr = atom_ident;
			return atom;
		}
	}

	std::optional<NodeExpr*> parse_expr() 
	{
		if (auto atom = parse_atom())
		{
			if (peek().has_value() && peek().value().type == TokenType::plus_sign)
			{
				auto bin_expr = m_allocator.alloc<NodeBinExpr>();

				auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();
				auto lhs_expr = m_allocator.alloc<NodeExpr>();
				lhs_expr->expr = atom.value();
				bin_expr_add->lhs = lhs_expr;
				
				consume();
				if (auto rhs = parse_expr())
				{
					bin_expr_add->rhs = rhs.value();
					bin_expr->bin_expr = bin_expr_add;
					auto expr = m_allocator.alloc<NodeExpr>();
					expr->expr = bin_expr;
					return expr;
				}
				else
				{
					std::cerr << "Expected expression" << std::endl;
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				auto expr = m_allocator.alloc<NodeExpr>();
				expr->expr = atom.value();
				return expr;
			}
		}
		else 
		{
			return {};
		}
	}

	std::optional<NodeStmt*> parse_stmt() 
	{
		if (peek().value().type == TokenType::exit && 
			peek(1).has_value() && peek(1).value().type == TokenType::open_paren) 
		{
			consume();
			consume();
			auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
			if (auto node_expr = parse_expr())
			{
				stmt_exit->expr = node_expr.value();
			}
			else {
				std::cerr << "Invalid errrrxpression" << std::endl;
				exit(EXIT_FAILURE);
			}

			if (peek().has_value() && peek().value().type == TokenType::close_paren)
			{
				consume();
			} 
			else
			{
				std::cerr << "Expected ')'" << std::endl;
				exit(EXIT_FAILURE);
			}
			
			if (peek().has_value() && peek().value().type == TokenType::semi)
			{
				consume();
			}
			else
			{
				std::cerr << "Expected ';'" << std::endl;
				exit(EXIT_FAILURE);
			}
			auto stmt_ret_node = m_allocator.alloc<NodeStmt>();
			stmt_ret_node->stmt = stmt_exit;
			return stmt_ret_node;
		} 
		else if (peek().has_value() && peek().value().type == TokenType::let 
			&& peek(1).has_value() && peek(1).value().type == TokenType::ident 
			&& peek(2).has_value() && peek(2).value().type == TokenType::equals) 
		{
			consume();
			auto stmt_let = m_allocator.alloc<NodeStmtLet>();
			stmt_let->ident = consume(); 
			consume();
			if (auto expr = parse_expr()) {
				stmt_let->expr = expr.value();
			}
			else 
			{
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			if (peek().has_value() && peek().value().type == TokenType::semi) 
			{
				consume();
			}
			else 
			{
				std::cerr << "Expected ';'" << std::endl;
				exit(EXIT_FAILURE);
			}
			auto stmt_ret_node = m_allocator.alloc<NodeStmt>();
			stmt_ret_node->stmt = stmt_let;
			return stmt_ret_node; 
		}
		else
		{
			return {};
		}
	}

	std::optional<NodeProgram> parse_prog() 
	{
		NodeProgram prog;
		
		while (peek().has_value())
		{
			if (auto stmt = parse_stmt())
			{
				prog.stmts.push_back(stmt.value());
			}
			else 
			{
				std::cerr << "Invalid Statement" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		return prog;
		
	}

private:	
	[[nodiscard]] inline std::optional<Token> peek(int offset = 0) const 
	{
		if (m_index + offset >= m_tokens.size())
		{
			return {};
		}
		else 
		{
			return m_tokens.at(m_index + offset);
		} 
		
	}
	
	inline Token consume() 
	{
		return m_tokens.at(m_index++);
	}

	const std::vector<Token> m_tokens;
	size_t m_index = 0;
	ArenaAllocator m_allocator;
};