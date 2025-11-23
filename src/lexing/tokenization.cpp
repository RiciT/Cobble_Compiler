#include "tokenization.hpp"

Tokenizer::Tokenizer(const std::string& src)
	: m_src(src)
{
}

std::vector<Token> Tokenizer::tokenize() {
	std::vector<Token> tokens;

	//reserve memory to prevent realloc
	//guess source length / 2 - heuristic guess
	tokens.reserve(m_src.length() / 2);

	while (peek().has_value())
	{
		// ReSharper disable once CppTooWideScopeInitStatement
		char next_char = peek().value();

		//idents and keywords
		if (std::isalpha(next_char)) { tokens.push_back(tokenize_ident_and_keyword()); }
		//int literals
		else if (std::isdigit(next_char)) { tokens.push_back(tokenize_int_lit()); }
		//single line comments
		else if (Test_Double_SingleCharTokens(next_char, TokenType::fslash_sign, TokenType::fslash_sign)) { skip_single_line_comment(); }
		//multi line comment
		else if (Test_Double_SingleCharTokens(next_char, TokenType::fslash_sign, TokenType::star_sign)) { skip_multi_line_comment(); }
		//==
		else if (Test_Double_SingleCharTokens(next_char, TokenType::equals, TokenType::equals))
			{ tokens.push_back(tokenize_multi_char_operator(2, TokenType::equals_equals)); }
		//!=
		else if (Test_Double_SingleCharTokens(next_char, TokenType::exclamation_point, TokenType::equals))
			{ tokens.push_back(tokenize_multi_char_operator(2, TokenType::not_equals)); }
		//>=
		else if (Test_Double_SingleCharTokens(next_char, TokenType::greater, TokenType::equals))
			{ tokens.push_back(tokenize_multi_char_operator(2, TokenType::greater_equals)); }
		//<=
		else if (Test_Double_SingleCharTokens(next_char, TokenType::less, TokenType::equals))
			{ tokens.push_back(tokenize_multi_char_operator(2, TokenType::less_equals)); }
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

Token Tokenizer::tokenize_ident_and_keyword()
{
	const size_t start_index = m_index;
	consume();
	while (peek().has_value() && std::isalnum(peek().value()))
	{
		consume();
	}
	std::string_view text = m_src.substr(start_index, m_index - start_index);

	if (const auto it = KeyWordTokens.find_by_key(text); it != KeyWordTokens.end())
	{
		return Token{ .type = it->second, .line = m_line_counter };
	}
	return Token{ .type = TokenType::ident, .value = text, .line = m_line_counter };
}

Token Tokenizer::tokenize_int_lit()
{
	const size_t start_index = m_index;
	consume();
	while (peek().has_value() && std::isdigit(peek().value()))
	{
		consume();
	}
	std::string_view text = m_src.substr(start_index, m_index - start_index);
	return Token{ .type = TokenType::int_lit, .value = text, .line = m_line_counter };
}

void Tokenizer::skip_single_line_comment()
{
	consume(); consume();
	do
	{
		consume();
	} while (peek().has_value() && peek().value() != '\n');
}

void Tokenizer::skip_multi_line_comment()
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

// ReSharper disable once CppDFAConstantParameter
Token Tokenizer::tokenize_multi_char_operator(const int numOfChar, const TokenType op)
{
	for (int i = 0; i < numOfChar; i++) consume();
	return Token{ .type = op, .line = m_line_counter };
}

[[nodiscard]] std::optional<char> Tokenizer::peek(const int offset) const
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
char Tokenizer::consume()
{
	if (m_src[m_index] == '\n') { m_line_counter++; };
	return m_src[m_index++];
}
bool Tokenizer::Test_Double_SingleCharTokens(const char next_char, const TokenType type1, const TokenType type2) const
{
	return SingleCharTokens.count(next_char) &&
					 SingleCharTokens.at(next_char) == type1 &&
					 peek(1).has_value() &&
					 SingleCharTokens.count(peek(1).value()) &&
					 SingleCharTokens.at(peek(1).value()) == type2;
}
