#pragma once //process only once if included

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "token.hpp"

class Tokenizer {
public:
	inline explicit Tokenizer(const std::string& src)
	: m_src(src)
	{
	}

	inline std::vector<Token> tokenize() {
		std::vector<Token> tokens;

		//reserve memory to prevent realloc
		//guess source length / 2 - heuristic guess
		tokens.reserve(m_src.length() / 2);

		while (peek().has_value())
		{
			// ReSharper disable once CppTooWideScopeInitStatement
			char next_char = peek().value();

			//idents and keywords
			if (std::isalpha(next_char))
			{
				const size_t start_index = m_index;
				consume();
				while (peek().has_value() && std::isalnum(peek().value()))
				{
					consume();
				}
				std::string_view text = m_src.substr(start_index, m_index - start_index);

				if (auto it = KeyWordTokens.find_by_key(text); it != KeyWordTokens.end())
				{
					tokens.push_back({ .type = it->second, .line = m_line_counter });
				}
				else
				{
					tokens.push_back({ .type = TokenType::ident, .value = text, .line = m_line_counter });
				}
			}
			//int literals
			else if (std::isdigit(next_char))
			{
				const size_t start_index = m_index;
				consume();
				while (peek().has_value() && std::isdigit(peek().value()))
				{
					consume();
				}
				std::string_view text = m_src.substr(start_index, m_index - start_index);
				tokens.push_back({ .type = TokenType::int_lit, .value = text, .line = m_line_counter });
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
				{ consume(); }
			//single char tokens
			else if (auto token_char = SingleCharTokens.find_by_key(next_char); token_char != SingleCharTokens.end())
			{
				consume();
				tokens.push_back({ .type = token_char->second, .line = m_line_counter });
			}
			else
			{
				std::cerr << "Could not tokenize something where peek() = " << peek().value() << " on line " << m_line_counter << std::endl;
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
			return m_src[m_index + offset];
		}

	}

	char consume()
	{
		if (m_src[m_index] == '\n') { m_line_counter++; };
		return m_src[m_index++];
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
	const std::string_view m_src; //m_ for members
	int m_index = 0;
};