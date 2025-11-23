#include "parser.hpp"

std::optional<NodeScope*> Parser::parse_scope()
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

std::optional<NodeIfPredicate*> Parser::parse_if_predicate()
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

std::optional<NodeStmt*> Parser::parse_stmt()
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
	if (peek().has_value() && peek().value().type == TokenType::def_)
	{
		bool is_expr = false;

		consume(); //def

		auto stmt_def = m_allocator.alloc<NodeStmtDef>();

		if (auto type = parse_base_type())
		{
			stmt_def->type = type.value();
		}
		else
		{
			std::cerr << "Expected type after 'def'" << std::endl;
			exit(EXIT_FAILURE);
		}

		//check for array syntax: int[expr]
		if (try_consume(TokenType::open_bracket))
		{
			auto size_expr = parse_expr();
			if (!size_expr.has_value())
			{
				std::cerr << "Expected array size expression" << std::endl;
				exit(EXIT_FAILURE);
			}

			stmt_def->array_size_expr = size_expr.value();
			stmt_def->type.is_array = true;

			try_consume(TokenType::close_bracket, "Expected ']'");
		}

		if (peek().has_value() && peek().value().type == TokenType::ident)
		{
			stmt_def->ident = consume();
		}
		else
		{
			std::cerr << "Expected identifier" << std::endl;
			exit(EXIT_FAILURE);
		}

		if (peek().value().type == TokenType::equals)
		{
			//equals sign
			consume();
			is_expr = true;
		}

		//arrays cant have initializer expressions for now
		if (is_expr && !stmt_def->type.is_array)
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
	if (peek().has_value() && peek().value().type == TokenType::func_)
	{
		//func token
		consume();
		auto stmt_func = m_allocator.alloc<NodeStmtFunc>();

		if (auto type = parse_base_type())
		{
			stmt_func->return_type = type.value();
		}
		else
		{
			std::cerr << "Expected return type after 'func'" << std::endl;
			exit(EXIT_FAILURE);
		}

		// Parse function name
		if (peek().has_value() && peek().value().type == TokenType::ident)
		{
			stmt_func->ident = consume();
		}
		else
		{
			std::cerr << "Expected function name" << std::endl;
			exit(EXIT_FAILURE);
		}

		try_consume(TokenType::open_paren, "Expected '('");

		std::vector<NodeFuncParam> param_idents;
		while (peek().has_value() && peek().value().type != TokenType::close_paren) {
			//handle params
			if (peek().has_value() && peek().value().type == TokenType::def_)
			{
				//def
				NodeFuncParam param;

				//expect: def int paramName
				try_consume(TokenType::def_, "Expected 'def' in parameter");

				if (auto type = parse_base_type())
				{
					param.type = type.value();
				}
				else
				{
					std::cerr << "Expected type in parameter" << std::endl;
					exit(EXIT_FAILURE);
				}

				if (peek().has_value() && peek().value().type == TokenType::ident)
				{
					param.ident = consume();
				}
				else
				{
					std::cerr << "Expected parameter name" << std::endl;
					exit(EXIT_FAILURE);
				}

				param_idents.push_back(param);
				if (peek().has_value() && peek().value().type == TokenType::comma)
				{
					consume();
					continue;
				}
				if (peek().has_value() && peek().value().type == TokenType::close_paren)
				{
					break;
				}
				std::cerr << "Expected ')' or for more parameters ',' in func def" << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		//close paren token
		try_consume(TokenType::close_paren, "Expected ')'");

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
		while (peek().has_value() && peek().value().type != TokenType::close_paren) {
			//handle params
			if (auto expr = parse_expr())
			{
				exprs.push_back(expr.value());
				if (peek().has_value() && peek().value().type == TokenType::comma)
				{
					consume();
					continue;
				}
				if (peek().has_value() && peek().value().type == TokenType::close_paren)
				{
					break;
				}
				std::cerr << "Expected ')' or for more parameters ',' in func call" << std::endl;
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
	//Array assignment
	if (peek().has_value() && peek().value().type == TokenType::ident &&
		peek(1).has_value() && peek(1).value().type == TokenType::open_bracket)
	{
		Token ident = try_consume(TokenType::ident).value();
		consume(); // open bracket

		auto index_expr = parse_expr();
		if (!index_expr.has_value())
		{
			std::cerr << "Expected index expression" << std::endl;
			exit(EXIT_FAILURE);
		}

		try_consume(TokenType::close_bracket, "Expected ']'");
		try_consume(TokenType::equals, "Expected '='");

		auto value_expr = parse_expr();
		if (!value_expr.has_value())
		{
			std::cerr << "Expected value expression" << std::endl;
			exit(EXIT_FAILURE);
		}

		try_consume(TokenType::semi, "Expected ';'");

		auto stmt_array_assign = m_allocator.alloc<NodeStmtArrayAssign>();
		stmt_array_assign->ident = ident;
		stmt_array_assign->index = index_expr.value();
		stmt_array_assign->value = value_expr.value();

		auto stmt = m_allocator.emplace<NodeStmt>(stmt_array_assign);
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
