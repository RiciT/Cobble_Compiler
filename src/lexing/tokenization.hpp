#pragma once //process only once if included

#include <string>
#include <utility>
#include <vector>

#include "token.hpp"

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
			// ReSharper disable once CppTooWideScopeInitStatement
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
						tokens.push_back({ .type = tokentype, .line = m_line_counter });
						buf.clear();
						break;
					}
				}
				if (!buf.empty())
				{
					tokens.push_back({ .type = TokenType::ident, .value = buf, .line = m_line_counter });
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
				tokens.push_back({ .type = TokenType::int_lit, .value = buf, .line = m_line_counter });
				buf.clear();
			}
			//single line comments
			else if (Test_Double_SingleCharTokens(next_char, TokenType::fslash_sign, TokenType::fslash_sign))
			{
				consume(); consume();
				do
				{
					consume();
				} while (peek().has_value() && peek().value() != '\n');
			}
			//multi line comment
			else if (Test_Double_SingleCharTokens(next_char, TokenType::fslash_sign, TokenType::star_sign))
			{
				consume(); consume();
				while (peek(1).has_value())
				{
					if (Test_Double_SingleCharTokens(peek().value(), TokenType::star_sign, TokenType::fslash_sign))
					{
						break;
					}

					consume();
				}
				if (peek().has_value()) { consume(); }
				if (peek().has_value()) { consume(); }
			}
			//==
			else if (Test_Double_SingleCharTokens(next_char, TokenType::equals, TokenType::equals))
				{ consume(); consume(); tokens.push_back({ .type = TokenType::equals_equals, .line = m_line_counter }); }
			//!=
			else if (Test_Double_SingleCharTokens(next_char, TokenType::exclamation_point, TokenType::equals))
				{ consume(); consume(); tokens.push_back({ .type = TokenType::not_equals, .line = m_line_counter }); }
			//>=
			else if (Test_Double_SingleCharTokens(next_char, TokenType::greater, TokenType::equals))
				{ consume(); consume(); tokens.push_back({ .type = TokenType::greater_equals, .line = m_line_counter }); }
			//<=
			else if (Test_Double_SingleCharTokens(next_char, TokenType::less, TokenType::equals))
				{ consume(); consume(); tokens.push_back({ .type = TokenType::less_equals, .line = m_line_counter }); }
			//empty space
			else if (std::isspace(next_char))
			{
				consume();
			}
			//single char tokens
			else if (auto token_char = SingleCharTokens.find_by_key(next_char); token_char != SingleCharTokens.end())
			{
				consume();
				tokens.push_back({ .type = token_char->second, .line = m_line_counter });
			}
			else
			{
				std::cerr << "Could not tokenize something where peek() = " << peek().value() << " and buf = " << buf << "on line " << m_line_counter << std::endl;
	 			exit(EXIT_FAILURE);
			}
		}
		m_index = 0;
		return tokens;
	};

private:
	[[nodiscard]] inline std::optional<char> peek(const int offset = 0) const
	{
		if (m_index + offset >= static_cast<int>(m_src.length()))
		{
			return {};
		}
		else
		{
			return m_src.at(m_index + offset);
		}

	}

	char consume()
	{
		if (m_src.at(m_index) == '\n') { m_line_counter++; };
		return m_src.at(m_index++);
	}

	bool Test_Double_SingleCharTokens(const char next_char, const TokenType type1, const TokenType type2) const
	{
		return SingleCharTokens.count(next_char) &&
						 SingleCharTokens.at(next_char) == type1 &&
						 peek(1).has_value() &&
						 SingleCharTokens.count(peek(1).value()) &&
						 SingleCharTokens.at(peek(1).value()) == type2;
	}

	size_t m_line_counter = 0; //for errors
	const std::string m_src; //m_ for members
	int m_index = 0;
};