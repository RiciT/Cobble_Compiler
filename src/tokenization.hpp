#pragma once //process only once if included

#include <string>
#include <utility>
#include <vector>
#include "unordered_bimap.hpp"

enum class TokenType
{
	//Atoms
	ident,
	int_lit,
	//Keywords
	exit_,
	def_,
	func_,
	if_,
	elseif_,
	else_,
	while_,
	print_,
	return_,
	//Single char tokens
	semi,
	open_paren,
	close_paren,
	equals,
	plus_sign,
	star_sign,
	dash_sign,
	fslash_sign,
	open_curly,
	close_curly,
	comma,
};

static const bidirectional_unordered_map<std::string, TokenType> KeyWordTokens = {
	{"exit", TokenType::exit_},
	{"def", TokenType::def_},
	{"func", TokenType::func_},
	{"if", TokenType::if_},
	{"elseif", TokenType::elseif_},
	{"else", TokenType::else_},
	{"while", TokenType::while_},
	{"print", TokenType::print_},
	{"return", TokenType::return_},
};

static const bidirectional_unordered_map<char, TokenType> SingleCharTokens = {
	{'(', TokenType::open_paren},
	{')', TokenType::close_paren},
	{'+', TokenType::plus_sign},
	{'-', TokenType::dash_sign},
	{'*', TokenType::star_sign},
	{'/', TokenType::fslash_sign},
	{'=', TokenType::equals},
	{';', TokenType::semi},
	{'{', TokenType::open_curly},
	{'}', TokenType::close_curly},
	{',', TokenType::comma},
};

inline std::optional<int> bin_precedence(const TokenType type) {
	switch(type) {
		case TokenType::plus_sign:
		case TokenType::dash_sign:
			return 0;
		case TokenType::star_sign:
		case TokenType::fslash_sign:
			return 1;
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
	inline explicit Tokenizer(std::string  src)
	: m_src(std::move(src))
	{
	}

	inline std::vector<Token> tokenize() {
		std::vector<Token> tokens;
		std::string buf;
		while (peek().has_value())
		{
			char next_char = peek().value();
			//idents and keywords
			if (std::isalpha(next_char))
			{
				buf.push_back(consume());
				while (peek().has_value() && std::isalnum(peek().value()))
				{
					buf.push_back(consume());
				}
				//really just a function
				for (const auto&[keyword, tokentype] : KeyWordTokens)
				{
					if (buf == keyword)
					{
						tokens.push_back({.type = tokentype});
						buf.clear();
						break;
					}
				}
				if (!buf.empty())
				{
					tokens.push_back({.type = TokenType::ident, .value = buf});
					buf.clear();
				}
			}
			//int literals
			else if (std::isdigit(next_char))
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
			//single line comments
			else if (SingleCharTokens.count(next_char) &&
         				SingleCharTokens.at(next_char) == TokenType::fslash_sign &&
         				peek(1).has_value() &&
         				SingleCharTokens.count(peek(1).value()) &&
         				SingleCharTokens.at(peek(1).value()) == TokenType::fslash_sign)
			{
				consume(); consume();
				do
				{
					consume();
				} while (peek().has_value() && peek().value() != '\n');
			}
			//multi line comment
			else if (SingleCharTokens.count(next_char) &&
         				SingleCharTokens.at(next_char) == TokenType::fslash_sign &&
         				peek(1).has_value() &&
         				SingleCharTokens.count(peek(1).value()) &&
         				SingleCharTokens.at(peek(1).value()) == TokenType::star_sign)
			{
				consume(); consume();
				while (peek(1).has_value())
				{
					if (SingleCharTokens.count(peek().value()) &&
            				SingleCharTokens.at(peek().value()) == TokenType::star_sign &&
            				peek(1).has_value() &&
            				SingleCharTokens.count(peek(1).value()) &&
            				SingleCharTokens.at(peek(1).value()) == TokenType::fslash_sign)
					{
						break;
					}

					consume();
				}
				if (peek().has_value()) { consume(); }
				if (peek().has_value()) { consume(); }
			}
			//empty space
			else if (std::isspace(next_char))
			{
				consume();
			}
			//single char tokens
			else if (auto token_char = SingleCharTokens.find_by_key(next_char); token_char != SingleCharTokens.end())
			{
				consume();
				tokens.push_back({.type = token_char->second});
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