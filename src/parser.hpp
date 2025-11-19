#pragma once

#include <variant>
#include <cassert>
#include <complex>

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

struct NodeScope {
	std::vector<NodeStmt*> stmts;
};

struct NodeStmtPrint {
	NodeExpr* expr;
};

struct NodeStmtExit {
	NodeExpr* expr;
};

struct NodeStmtDef {
	Token ident;
	std::optional<NodeExpr*> expr{};
};

struct NodeStmtFunc {
	Token ident;
	std::optional<std::vector<Token>> params;
	NodeScope* scope{};
};

struct NodeStmtFuncCall {
	Token ident;
	std::optional<std::vector<NodeExpr*>> exprs;
};

struct NodeStmtReturn {
	std::optional<NodeExpr*> expr;
};

struct NodeIfPredicate;

struct NodeIfPredElseIf {
	NodeExpr* expr{};
	NodeScope* scope{};
	std::optional<NodeIfPredicate*> ifpred;
};

struct NodeIfPredElse {
	NodeScope* scope;
};

struct NodeIfPredicate {
	std::variant<NodeIfPredElseIf*, NodeIfPredElse*> ifpred;
};

struct NodeStmtIf {
	NodeExpr* expr{};
	NodeScope* scope{};
	std::optional<NodeIfPredicate*> ifpred;
};

struct NodeStmtAssign {
	Token ident;
	NodeExpr* expr{};
};

struct NodeStmtWhile {
	NodeExpr* expr;
	NodeScope* scope;
};

struct NodeStmt {
	std::variant<NodeStmtExit*, NodeStmtPrint*, NodeStmtDef*, NodeScope*, NodeStmtIf*,
		NodeStmtAssign*, NodeStmtWhile*, NodeStmtFunc*, NodeStmtFuncCall*, NodeStmtReturn*> stmt;
};

struct NodeProgram {
	std::vector<NodeStmt*> stmts;
};
#pragma endregion

class Parser {
#pragma region  //public:
public:
	explicit Parser(std::vector<Token> tokens)
		: m_tokens(std::move(tokens)),
		m_allocator(1024 * 1024 * 4) //4 Mb
	{
	}

	std::optional<NodeAtom*> parse_atom() 
	{
		if (const auto int_lit = try_consume(TokenType::int_lit))
		{
			auto atom_int_lit = m_allocator.alloc<NodeAtomIntLit>();
			atom_int_lit->int_lit = int_lit.value();
			auto atom = m_allocator.alloc<NodeAtom>();
			atom->primary_expr = atom_int_lit;
			return atom;
		}
		if (const auto ident = try_consume(TokenType::ident))
		{
			auto atom_ident = m_allocator.alloc<NodeAtomIdent>();
			atom_ident->ident = ident.value();
			auto atom = m_allocator.alloc<NodeAtom>();
			atom->primary_expr = atom_ident;
			return atom;
		}
		if (auto open_paren = try_consume(TokenType::open_paren))
		{
			const auto expr = parse_expr();
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

	std::optional<NodeExpr*> parse_expr(const int minimum_precedence = 0)
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
				prec = bin_precedence(curr_tok->type);
				if (!prec.has_value() || prec < minimum_precedence)
					break;
			}
			else
			{
				break;
			}

			auto [type, value] = consume();
			const int next_minimum_precedence = prec.value() + 1;
			auto expr_rhs = parse_expr(next_minimum_precedence);

			if (!expr_rhs.has_value())
			{
				std::cerr << "Unable to parse expression" << std::endl;
				exit(EXIT_FAILURE);
			}

			auto expr = m_allocator.alloc<NodeBinExpr>();
			const auto node_expr_lhs = m_allocator.alloc<NodeExpr>();
			if (type == TokenType::plus_sign)
			{
				auto add = m_allocator.alloc<NodeBinExprAdd>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				add->lhs = node_expr_lhs;
				add->rhs = expr_rhs.value();
				expr->bin_expr = add;
			}
			else if (type == TokenType::dash_sign)
			{
				auto sub = m_allocator.alloc<NodeBinExprSub>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				sub->lhs = node_expr_lhs;
				sub->rhs = expr_rhs.value();
				expr->bin_expr = sub;
			}
			else if (type == TokenType::star_sign)
			{
				auto mult = m_allocator.alloc<NodeBinExprMult>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				mult->lhs = node_expr_lhs;
				mult->rhs = expr_rhs.value();
				expr->bin_expr = mult;
			}
			else if (type == TokenType::fslash_sign)
			{
				auto div = m_allocator.alloc<NodeBinExprDiv>();
				
				node_expr_lhs->expr = expr_lhs->expr;
				div->lhs = node_expr_lhs;
				div->rhs = expr_rhs.value();
				expr->bin_expr = div;
			}
			else { assert(false); } //Should be unreachable
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
		//Exit statement
		if (peek().value().type == TokenType::exit_ &&
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
		//Define variable
		if (peek().has_value() && peek().value().type == TokenType::def_
			&& peek(1).has_value() && peek(1).value().type == TokenType::ident)
		{
			bool is_expr = false;
			consume();
			auto stmt_def = m_allocator.alloc<NodeStmtDef>();
			stmt_def->ident = consume();

			if (peek().value().type == TokenType::equals)
			{
				//equals sign
				consume();
				is_expr = true;
			}

			if (is_expr)
			{
				if (auto expr = parse_expr()) {
					stmt_def->expr = expr.value();
				}
				else
				{
					std::cerr << "Invalid expression" << std::endl;
					exit(EXIT_FAILURE);
				}
			}

			try_consume(TokenType::semi, "Expected ';'");

			auto stmt_ret_node = m_allocator.alloc<NodeStmt>();
			stmt_ret_node->stmt = stmt_def;
			return stmt_ret_node; 
		}
		//Function definition
		if (peek().has_value() && peek().value().type == TokenType::func_
			&& peek(1).has_value() && peek(1).value().type == TokenType::ident
			&& peek(2).has_value() && peek(2).value().type == TokenType::open_paren)
		{
			//func token
			consume();
			auto stmt_func = m_allocator.alloc<NodeStmtFunc>();
			stmt_func->ident = consume();

			//open paren token
			consume();

			std::vector<Token> param_idents;
			while (true) {
				//handle params
				if (peek().has_value() && peek().value().type == TokenType::def_
					&& peek(1).has_value() && peek(1).value().type == TokenType::ident)
				{
					//def
					consume();
					param_idents.push_back(consume());
					if (peek().has_value() && peek().value().type == TokenType::close_paren)
					{
						break;
					}
					if (peek().has_value() && peek().value().type == TokenType::comma)
					{
						consume();
						continue;
					}
					std::cerr << "Expected ')' or for more parameters ','" << std::endl;
					exit(EXIT_FAILURE);
				}
			}
			//close paren token
			consume();

			if (!param_idents.empty())
				stmt_func->params = param_idents;

			if (auto scope = parse_scope())
			{
				stmt_func->scope = scope.value();
			}
			else
			{
				std::cerr << "Invalid scope" << std::endl;
				exit(EXIT_FAILURE);
			}
			auto stmt = m_allocator.emplace<NodeStmt>(stmt_func);
			return stmt;
		}
		//Function call
		if (peek().has_value() && peek().value().type == TokenType::ident &&
			peek(1).has_value() && peek(1).value().type == TokenType::open_paren)
		{
			auto stmt_func_call = m_allocator.alloc<NodeStmtFuncCall>();
			stmt_func_call->ident = consume();

			//open paren
			consume();

			std::vector<NodeExpr*> exprs;
			while (true) {
				//handle params
				if (auto expr = parse_expr())
				{
					exprs.push_back(expr.value());
					if (peek().has_value() && peek().value().type == TokenType::close_paren)
					{
						break;
					}
					if (peek().has_value() && peek().value().type == TokenType::comma)
					{
						consume();
						continue;
					}
					std::cerr << "Expected ')' or for more parameters ','" << std::endl;
					exit(EXIT_FAILURE);
				}
			}

			//close paren token
			try_consume(TokenType::close_paren, "Expected ')'");

			//semi token
			try_consume(TokenType::semi, "Expected ';'");

			if (!exprs.empty())
				stmt_func_call->exprs = exprs;

			auto stmt = m_allocator.emplace<NodeStmt>(stmt_func_call);
			return stmt;
		}
		//Return
		if (peek().has_value() && peek().value().type == TokenType::return_)
		{
			consume();
			auto stmt_return = m_allocator.alloc<NodeStmtReturn>();
			if (auto expr = parse_expr())
			{
				stmt_return->expr = expr.value();
			}
			try_consume(TokenType::semi, "Expected ';'");

			auto stmt = m_allocator.emplace<NodeStmt>(stmt_return);
			return stmt;
		}
		//Variable assignment
		if (peek().has_value() && peek().value().type == TokenType::ident &&
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
		//Scope
		if (peek().has_value() && peek().value().type == TokenType::open_curly)
		{
			if (auto scope = parse_scope())
			{
				auto stmt = m_allocator.alloc<NodeStmt>();
				stmt->stmt = scope.value();
				return stmt;
			}
			std::cerr << "Invalid scope" << std::endl;
			exit(EXIT_FAILURE);
		}
		//If
		if (auto if_ = try_consume(TokenType::if_))
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
		//While
		if (auto while_ = try_consume(TokenType::while_))
		{
			try_consume(TokenType::open_paren, "Expected '('");
			auto stmt_while = m_allocator.alloc<NodeStmtWhile>();
			if (auto expr = parse_expr())
			{
				stmt_while->expr = expr.value();
			}
			else 
			{
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected ')'");
			if (auto scope = parse_scope())
			{
				stmt_while->scope = scope.value();
			}
			else
			{
				std::cerr << "Invalid scope" << std::endl;
				exit(EXIT_FAILURE);
			}

			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->stmt = stmt_while;
			return stmt;
		}
		//Print
		if (auto print_ = try_consume(TokenType::print_))
		{
			try_consume(TokenType::open_paren, "Expected '('");
			auto stmt_print = m_allocator.alloc<NodeStmtPrint>();
			if (auto expr = parse_expr()) {
				stmt_print->expr = expr.value();
			}
			else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected ')'");
			try_consume(TokenType::semi, "Expected ';'");

			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->stmt = stmt_print;
			return stmt;
		}

		return {};
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
	[[nodiscard]] std::optional<Token> peek(const int offset = 0) const
	{
		if (m_index + offset >= m_tokens.size())
		{
			return {};
		}
		return m_tokens.at(m_index + offset);
	}
	
	inline Token consume() 
	{
		return m_tokens.at(m_index++);
	}

	Token try_consume(const TokenType type, const std::string& err_msg)
	{
		if (peek().has_value() && peek().value().type == type)
		{
			return consume();
		}
			std::cerr << err_msg << std::endl;
			exit(EXIT_FAILURE);
	}

	std::optional<Token> try_consume(const TokenType type)
	{
		if (peek().has_value() && peek().value().type == type)
		{
			return consume();
		}
		return {};
	}

	const std::vector<Token> m_tokens;
	size_t m_index = 0;
	ArenaAllocator m_allocator;
	#pragma endregion
};