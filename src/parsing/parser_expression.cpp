#include "parser.hpp"
#include "operator_precedence.hpp"

std::optional<NodeAtom*> Parser::parse_atom()
{
	if (const auto int_lit = try_consume(TokenType::int_lit))
	{
		auto atom_int_lit = m_allocator.alloc<NodeAtomIntLit>();
		atom_int_lit->int_lit = int_lit.value();
		auto atom = m_allocator.alloc<NodeAtom>();
		atom->primary_expr = atom_int_lit;
		return atom;
	}
	if (const auto true_lit = try_consume(TokenType::true_))
	{
		const auto atom_bool_lit = m_allocator.alloc<NodeAtomBoolLit>();
		atom_bool_lit->bool_lit = true_lit.value();
		auto atom = m_allocator.alloc<NodeAtom>();
		atom->primary_expr = atom_bool_lit;
		return atom;
	}
	if (const auto false_lit = try_consume(TokenType::false_))
	{
		const auto atom_bool_lit = m_allocator.alloc<NodeAtomBoolLit>();
		atom_bool_lit->bool_lit = false_lit.value();
		auto atom = m_allocator.alloc<NodeAtom>();
		atom->primary_expr = atom_bool_lit;
		return atom;
	}
	//need this before the normal variable!!!!
	if (peek().has_value() && peek().value().type == TokenType::ident &&
		peek(1).has_value() && peek(1).value().type == TokenType::open_bracket)
	{
		Token ident = try_consume(TokenType::ident, "Expected identifier");
		try_consume(TokenType::open_bracket, "Expected '['");

		auto index_expr = parse_expr();
		if (!index_expr.has_value())
		{
			std::cerr << "Expected index expression" << std::endl;
			exit(EXIT_FAILURE);
		}

		try_consume(TokenType::close_bracket, "Expected ']'");

		auto atom_array_access = m_allocator.alloc<NodeAtomArrayAccess>();
		atom_array_access->ident = ident;
		atom_array_access->index = index_expr.value();

		auto atom = m_allocator.alloc<NodeAtom>();
		atom->primary_expr = atom_array_access;
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

std::optional<VarType> Parser::parse_base_type()
{
	if (auto [type, _] = consume(); VariableBaseTypes.contains_value(type))
	{
		return VarType{ .base = VariableBaseTypes.at_value(type) };
	}

	return {};
}

std::optional<NodeFuncCallExpr*> Parser::parse_expr_func_call()
{
	auto expr_func_call = m_allocator.alloc<NodeFuncCallExpr>();
	expr_func_call->ident = consume();
	consume();  // open paren

	std::vector<NodeExpr*> exprs;
	while (peek().has_value() && peek().value().type != TokenType::close_paren) {
		if (auto expr = parse_expr())
		{
			exprs.push_back(expr.value());
			if (try_consume(TokenType::comma)) continue;
			if (peek().has_value() && peek().value().type == TokenType::close_paren) break;
			std::cerr << "Expected ')' or ',' in func call" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	try_consume(TokenType::close_paren, "Expected ')'");

	if (!exprs.empty())
		expr_func_call->exprs = exprs;

	return expr_func_call;
}

std::optional<NodeExpr*> Parser::parse_expr(const int minimum_precedence)
{
	NodeExpr* expr_lhs = nullptr;

	if (peek().has_value() && peek().value().type == TokenType::ident
		&& peek(1).has_value() && peek(1).value().type == TokenType::open_paren)
	{
		std::optional<NodeFuncCallExpr*> func_call_lhs = parse_expr_func_call();
		if (!func_call_lhs.has_value()) { return {}; }
		expr_lhs = m_allocator.alloc<NodeExpr>();
		expr_lhs->expr = func_call_lhs.value();
	}
	else
	{
		//try atom
		std::optional<NodeAtom*> atom_lhs = parse_atom();
		if (!atom_lhs.has_value()) { return {}; }
		expr_lhs = m_allocator.alloc<NodeExpr>();
		expr_lhs->expr = atom_lhs.value();
	}

	//precedence climbing from Eli Bendersky (https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing)
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
		//i dont yet know how but somehow we need a map from operator<->NodeBinExpr
		//as all of these are the same
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
		else if (type == TokenType::equals_equals)
		{
			auto eq = m_allocator.alloc<NodeBinExprEq>();

			node_expr_lhs->expr = expr_lhs->expr;
			eq->lhs = node_expr_lhs;
			eq->rhs = expr_rhs.value();
			expr->bin_expr = eq;
		}
		else if (type == TokenType::not_equals)
		{
			auto neq = m_allocator.alloc<NodeBinExprNotEq>();

			node_expr_lhs->expr = expr_lhs->expr;
			neq->lhs = node_expr_lhs;
			neq->rhs = expr_rhs.value();
			expr->bin_expr = neq;
		}
		else if (type == TokenType::greater_equals)
		{
			auto geq = m_allocator.alloc<NodeBinExprGreaterEq>();

			node_expr_lhs->expr = expr_lhs->expr;
			geq->lhs = node_expr_lhs;
			geq->rhs = expr_rhs.value();
			expr->bin_expr = geq;
		}
		else if (type == TokenType::less_equals)
		{
			auto leq = m_allocator.alloc<NodeBinExprLessEq>();

			node_expr_lhs->expr = expr_lhs->expr;
			leq->lhs = node_expr_lhs;
			leq->rhs = expr_rhs.value();
			expr->bin_expr = leq;
		}
		else if (type == TokenType::greater)
		{
			auto gt = m_allocator.alloc<NodeBinExprGreater>();

			node_expr_lhs->expr = expr_lhs->expr;
			gt->lhs = node_expr_lhs;
			gt->rhs = expr_rhs.value();
			expr->bin_expr = gt;
		}
		else if (type == TokenType::less)
		{
			auto lt = m_allocator.alloc<NodeBinExprLess>();

			node_expr_lhs->expr = expr_lhs->expr;
			lt->lhs = node_expr_lhs;
			lt->rhs = expr_rhs.value();
			expr->bin_expr = lt;
		}
		else { assert(false); } //Should be unreachable
		expr_lhs->expr = expr;

	}
	return expr_lhs;
}
