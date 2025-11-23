#include "parser.hpp"

Parser::Parser(std::vector<Token> tokens, ErrorHandler& error_handler)
		: m_tokens(std::move(tokens)),
		m_allocator(1024 * 1024 * 4), //4 Mb
		m_error_handler(error_handler)
	{
	}

std::optional<NodeProgram> Parser::parse_prog()
{
	NodeProgram prog;

	while (peek().has_value())
	{
		if (auto stmt = parse_stmt())
		{
			prog.stmts.push_back(stmt.value());
		}
		else { break; }
		//else { m_error_handler.error("Invalid Statement"); }
	}
	return prog;

}

//helpers
[[nodiscard]] std::optional<Token> Parser::peek(const int offset) const
{
	if (m_index + offset >= m_tokens.size()) { return {}; }
	return m_tokens.at(m_index + offset);
}

Token Parser::consume()
{
	return m_tokens.at(m_index++);
}

std::optional<Token> Parser::try_consume(const TokenType type, const std::string& err_msg)
{
	if (peek().has_value() && peek().value().type == type) { return consume(); }

	//semi acts as a placeholder default token type here
	m_error_handler.report(err_msg, peek().value_or(Token{ .type = TokenType::semi, .line = 0 }));
	return {}; //empty signals failure as in the other try_consume
}

std::optional<Token> Parser::try_consume(const TokenType type)
{
	if (peek().has_value() && peek().value().type == type) { return consume(); }
	return {}; //empty to signal failure
}