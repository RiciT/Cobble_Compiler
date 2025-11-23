#pragma once //process only once if included

#include <string>
#include <string_view>
#include <vector>

#include "token.hpp"

class Tokenizer {
public:
	explicit Tokenizer(const std::string& src);
	std::vector<Token> tokenize();

private:
	Token tokenize_ident_and_keyword();
	Token tokenize_int_lit();
	void skip_single_line_comment();
	void skip_multi_line_comment();
	Token tokenize_multi_char_operator(const int numOfChar, const TokenType op);

	[[nodiscard]] std::optional<char> peek(const int offset = 0) const;
	char consume();
	bool Test_Double_SingleCharTokens(const char next_char, const TokenType type1, const TokenType type2) const;

	size_t m_line_counter = 0; //for errors
	const std::string_view m_src; //m_ for members
	int m_index = 0;
};