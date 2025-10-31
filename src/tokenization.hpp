#pragma once //process only once if included

#include <string>
#include <vector>

enum class TokenType 
{
	exit,
	int_lit,
	semi,
	open_paren,
	close_paren,
	ident,
	def,
	equals,
	plus_sign,
	mult_sign,
};

std::optional<int> bin_prec(TokenType type) {
	switch(type) {
		case TokenType::plus_sign:
			return 1;
		case TokenType::mult_sign:
			return 2;
		default:
			return {};
	}
}

struct Token 
{
	TokenType type;
	std::optional<std::string> value {};
};

class Tokenizer {
public:
	inline explicit Tokenizer(const std::string& src) 
	: m_src(std::move(src)) 
	{
	}

	inline std::vector<Token> tokenize() {	
		std::vector<Token> tokens;
		std::string buf;
		while (peek().has_value())
		{
			if (std::isalpha(peek().value()))
			{
				buf.push_back(consume());
				while (peek().has_value() && std::isalnum(peek().value()))
				{
					buf.push_back(consume());
				}
				if (buf == "exit")
				{
					tokens.push_back({.type = TokenType::exit});
					buf.clear();
					continue;
				}
				else if (buf == "def")
				{
					tokens.push_back({.type = TokenType::def});
					buf.clear();
					continue;
				}
				else
				{
					tokens.push_back({.type = TokenType::ident, .value = buf});
					buf.clear();
					continue;
				}
				
			}
			else if (std::isdigit(peek().value()))
			{
				buf.push_back(consume());
				while (peek().has_value() && std::isdigit(peek().value()))
				{
					buf.push_back(consume());
				}
				tokens.push_back({.type = TokenType::int_lit, .value = buf});
				buf.clear();
				continue;
			}
			else if (peek().value() == '(') {
				consume();
				tokens.push_back({.type = TokenType::open_paren});
				continue;
			}
			else if (peek().value() == ')') {
				consume();
				tokens.push_back({.type = TokenType::close_paren});
				continue;
			}
			else if (peek().value() == '=') 
			{
				consume();
				tokens.push_back({.type = TokenType::equals});
				continue;
			}
			else if (peek().value() == '+')
			{
				consume();
				tokens.push_back({ .type = TokenType::plus_sign});
				continue;
			}
			else if (peek().value() == '*')
			{
				consume();
				tokens.push_back({ .type = TokenType::mult_sign});
				continue;
			}
			else if (peek().value() == '*')
			{
				consume();
				tokens.push_back({ .type = TokenType::mult_sign});
				continue;
			}
			else if (peek().value() == ';') 
			{
				consume();
				tokens.push_back({.type = TokenType::semi});
				continue;
			}
			else if (std::isspace(peek().value())) 
			{
				consume();
				continue;
			}
			else 
			{
				std::cerr << "You messed up" << std::endl;
	 			exit(EXIT_FAILURE);
			}
		}
		m_index = 0;
		return tokens;
	};

private:
	[[nodiscard]] inline std::optional<char> peek(int offset = 0) const 
	{
		if (m_index + offset >= m_src.length())
		{
			return {};
		}
		else 
		{
			return m_src.at(m_index + offset);
		} 
		
	}
	
	inline char consume() 
	{
		return m_src.at(m_index++);
	}

	const std::string m_src; //m_ for members 
	int m_index = 0;
};
