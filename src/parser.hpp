#pragma once

#include <variant>
#include <cassert>

#include "tokenization.hpp"
#include "arena_allocator.hpp"

#pragma region Nodes
struct NodeAtomIntLit {
	Token int_lit;
};

struct NodeAtomIdent {
	Token ident;
};

struct NodeExpr;

struct NodeAtomParen {
	NodeExpr* expr;
};

struct NodeBinExprAdd {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprMult {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprSub {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprDiv {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExpr {
	std::variant<NodeBinExprAdd*, NodeBinExprMult*, NodeBinExprDiv*, NodeBinExprSub*> bin_expr;
};

struct NodeAtom {
	std::variant<NodeAtomIntLit*, NodeAtomIdent*, NodeAtomParen*> primary_expr;
};

struct NodeExpr {
	std::variant<NodeAtom*, NodeBinExpr*> expr;
};

struct NodeStmt;

struct NodeStmtExit {
	NodeExpr* expr;
};

struct NodeStmtDef {
	Token ident;
	NodeExpr* expr;
};

struct NodeScope {
	std::vector<NodeStmt*> stmts;
};

struct NodeIfPredicate;

struct NodeIfPredElseIf {
	NodeExpr* expr;
	NodeScope* scope;
	std::optional<NodeIfPredicate*> ifpred;
};

struct NodeIfPredElse {
	NodeScope* scope;
};

struct NodeIfPredicate {
	std::variant<NodeIfPredElseIf*, NodeIfPredElse*> ifpred;
};

struct NodeStmtIf {
	NodeExpr* expr;
	NodeScope* scope;
	std::optional<NodeIfPredicate*> ifpred;
};

struct NodeStmtAssign {
	Token  ident;
	NodeExpr* expr;
};

struct NodeStmt {
	std::variant<NodeStmtExit*, NodeStmtDef*, NodeScope*, NodeStmtIf*, NodeStmtAssign*> stmt;
};

struct NodeProgram {
	std::vector<NodeStmt*> stmts;
};
#pragma endregion

class Parser {
#pragma region  //public:
public:
	inline explicit Parser(std::vector<Token> tokens)
		: m_tokens(std::move(tokens)),
		m_allocator(1024 * 1024 * 4) //4 Mb
	{
	}

	std::optional<NodeAtom*> parse_atom() 
	{
		if (auto int_lit = try_consume(TokenType::int_lit)) 
		{
			auto atom_int_lit = m_allocator.alloc<NodeAtomIntLit>();
			atom_int_lit->int_lit = int_lit.value();
			auto atom = m_allocator.alloc<NodeAtom>();
			atom->primary_expr = atom_int_lit;
			return atom;
		}
		else if (auto ident = try_consume(TokenType::ident)) 
		{
			auto atom_ident = m_allocator.alloc<NodeAtomIdent>();
			atom_ident->ident = ident.value();
			auto atom = m_allocator.alloc<NodeAtom>();
			atom->primary_expr = atom_ident;
			return atom;
		}
		else if (auto open_paren = try_consume(TokenType::open_paren))
		{
			auto expr = parse_expr();
			if (!expr.has_value()) 
			{
				std::cerr << "Expected expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected ')'");
			auto atom_paren = m_allocator.alloc<NodeAtomParen>();
			atom_paren->expr = expr.value();
			auto atom = m_allocator.alloc<NodeAtom>();
			atom->primary_expr = atom_paren;
			return atom;
		}
		return {};
	}

	std::optional<NodeExpr*> parse_expr(int minimum_precedence = 0) 
	{
		//precedence climbing from Eli Bendersky (https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing)
		std::optional<NodeAtom*> atom_lhs = parse_atom();
		if (!atom_lhs.has_value()) { return {}; }

		auto expr_lhs = m_allocator.alloc<NodeExpr>();
		expr_lhs->expr = atom_lhs.value();

		while (true)
		{
			std::optional<Token> curr_tok = peek();
			std::optional<int> prec; 
			if (curr_tok.has_value()) 
			{
				prec = bin_prec(curr_tok->type);
				if (!prec.has_value() || prec < minimum_precedence)
					break;
			}
			else
			{
				break;
			}

			Token op = consume();
			int next_minimum_precedence = prec.value() + 1;
			auto expr_rhs = parse_expr(next_minimum_precedence);

			if (!expr_rhs.has_value())
			{
				std::cerr << "Unable to parse expression" << std::endl;
				exit(EXIT_FAILURE);
			}

			auto expr = m_allocator.alloc<NodeBinExpr>();
			auto node_expr_lhs = m_allocator.alloc<NodeExpr>();
			if (op.type == TokenType::plus_sign)
			{
				auto add = m_allocator.alloc<NodeBinExprAdd>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				add->lhs = node_expr_lhs;
				add->rhs = expr_rhs.value();
				expr->bin_expr = add;
			}
			else if (op.type == TokenType::dash_sign)
			{
				auto sub = m_allocator.alloc<NodeBinExprSub>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				sub->lhs = node_expr_lhs;
				sub->rhs = expr_rhs.value();
				expr->bin_expr = sub;
			}
			else if (op.type == TokenType::star_sign)
			{
				auto mult = m_allocator.alloc<NodeBinExprMult>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				mult->lhs = node_expr_lhs;
				mult->rhs = expr_rhs.value();
				expr->bin_expr = mult;
			}
			else if (op.type == TokenType::fslash_sign)
			{
				auto div = m_allocator.alloc<NodeBinExprDiv>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				div->lhs = node_expr_lhs;
				div->rhs = expr_rhs.value();
				expr->bin_expr = div;
			}
			else { assert(false); } //Unreachable
			expr_lhs->expr = expr;

		}
		return expr_lhs;
	}

	std::optional<NodeScope*> parse_scope()
	{
		if (!try_consume(TokenType::open_curly).has_value()) { return {}; }
		auto scope = m_allocator.alloc<NodeScope>();
		while (auto stmt = parse_stmt())
		{
			scope->stmts.push_back(stmt.value());
		}
		try_consume(TokenType::close_curly, "Expected '}'");
		return scope;
	}

	std::optional<NodeIfPredicate*> parse_if_predicate()
	{
		if (try_consume(TokenType::elseif_))
		{
			try_consume(TokenType::open_paren, "Expected '" + std::to_string(SingleCharTokens.at_value(TokenType::open_paren)) + "'");
			auto elif_ = m_allocator.alloc<NodeIfPredElseIf>();
			if (const auto expr = parse_expr())
			{
				elif_->expr = expr.value();
			}
			else
			{
				std::cerr << "Expected expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			
			try_consume(TokenType::close_paren, "Expected '" + std::to_string(SingleCharTokens.at_value(TokenType::close_paren)) + "'");
			if (const auto scope = parse_scope())
			{
				elif_->scope = scope.value();
			}
			else
			{
				std::cerr << "Expected scope" << std::endl;
				exit(EXIT_FAILURE);
			}
			elif_->ifpred = parse_if_predicate();
			auto pred = m_allocator.emplace<NodeIfPredicate>(elif_);
			return pred;
		}

		if (try_consume(TokenType::else_))
		{
			auto else_ = m_allocator.alloc<NodeIfPredElse>();
			if (const auto scope = parse_scope())
			{
				else_->scope = scope.value();
			}
			else 
			{
				std::cerr << "Expected scope" << std::endl;
				exit(EXIT_FAILURE);	
			}
			auto pred = m_allocator.emplace<NodeIfPredicate>(else_);
			return pred;
		}
		return {};
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
			try_consume(TokenType::close_paren, "Expected ')'");
			try_consume(TokenType::semi, "Expected ';'");

			auto stmt_ret_node = m_allocator.alloc<NodeStmt>();
			stmt_ret_node->stmt = stmt_exit;
			return stmt_ret_node;
		} 
		else if (peek().has_value() && peek().value().type == TokenType::def 
			&& peek(1).has_value() && peek(1).value().type == TokenType::ident 
			&& peek(2).has_value() && peek(2).value().type == TokenType::equals) 
		{
			consume();
			auto stmt_def = m_allocator.alloc<NodeStmtDef>();
			stmt_def->ident = consume(); 
			consume();
			if (auto expr = parse_expr()) {
				stmt_def->expr = expr.value();
			}
			else 
			{
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			
			try_consume(TokenType::semi, "Expected ';'");

			auto stmt_ret_node = m_allocator.alloc<NodeStmt>();
			stmt_ret_node->stmt = stmt_def;
			return stmt_ret_node; 
		}
		else if (peek().has_value() && peek().value().type == TokenType::ident && 
			peek(1).has_value() && peek(1).value().type == TokenType::equals)
		{
			const auto assign	= m_allocator.alloc<NodeStmtAssign>();
			assign->ident = consume(); //ident
			consume(); //=

			if (const auto expr = parse_expr())
			{
				assign->expr = expr.value();
			}
			else
			{
				std::cerr << "Expected expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::semi, "Expected ';'");
			auto stmt = m_allocator.emplace<NodeStmt>(assign);
			return stmt;
		}
		else if (peek().has_value() && peek().value().type == TokenType::open_curly)
		{
			if (auto scope = parse_scope())
			{
				auto stmt = m_allocator.alloc<NodeStmt>();
				stmt->stmt = scope.value();
				return stmt;
			}
			else
			{
				std::cerr << "Invalid scope" << std::endl;
				exit(EXIT_FAILURE);
			}
			
		}
		else if (auto if_ = try_consume(TokenType::if_))
		{
			try_consume(TokenType::open_paren, "Expected '('");
			auto stmt_if = m_allocator.alloc<NodeStmtIf>();
			if (auto expr = parse_expr())
			{
				stmt_if->expr = expr.value();
			}
			else 
			{
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected ')'");
			if (auto scope = parse_scope())
			{
				stmt_if->scope = scope.value();
			}
			else
			{
				std::cerr << "Invalid scope" << std::endl;
				exit(EXIT_FAILURE);
			}

			stmt_if->ifpred = parse_if_predicate();

			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->stmt = stmt_if;
			return stmt;
			
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

#pragma endregion
#pragma region //private: 
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

	inline Token try_consume(TokenType type, std::string err_msg)
	{
		if (peek().has_value() && peek().value().type == type)
		{
			return consume();
		}
		else
		{
			std::cerr << err_msg << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	inline std::optional<Token> try_consume(TokenType type)
	{
		if (peek().has_value() && peek().value().type == type)
		{
			return consume();
		}
		else
		{
			return {};
		}
	}

	const std::vector<Token> m_tokens;
	size_t m_index = 0;
	ArenaAllocator m_allocator;
	#pragma endregion
};