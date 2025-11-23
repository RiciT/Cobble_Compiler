#include "parser.hpp"

Parser::Parser(std::vector<Token> tokens)
		: m_tokens(std::move(tokens)),
		m_allocator(1024 * 1024 * 4) //4 Mb
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
		else
		{
			std::cerr << "Invalid Statement" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	return prog;

}

//helpers
[[nodiscard]] std::optional<Token> Parser::peek(const int offset) const
{
	if (m_index + offset >= m_tokens.size())
	{
		return {};
	}
	return m_tokens.at(m_index + offset);
}

Token Parser::consume()
{
	return m_tokens.at(m_index++);
}

Token Parser::try_consume(const TokenType type, const std::string& err_msg)
{
	if (peek().has_value() && peek().value().type == type)
	{
		return consume();
	}
		std::cerr << err_msg << std::endl;
		exit(EXIT_FAILURE);
}

std::optional<Token> Parser::try_consume(const TokenType type)
{
	if (peek().has_value() && peek().value().type == type)
	{
		return consume();
	}
	return {};
}