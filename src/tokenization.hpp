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
	true_,
	false_,
	//types
	int_,
	char_,
	bool_,
	//Boolean operators
	equals_equals,
	not_equals,
	greater_equals,
	less_equals,
	greater,
	less,
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
	single_quote,
	double_quote,
	exclamation_point,
	open_bracket,
	close_bracket
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
	{"int", TokenType::int_},
	{"char", TokenType::char_},
	{"bool", TokenType::bool_},
	{"true", TokenType::true_},
	{"false", TokenType::false_},
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
	{'\'', TokenType::single_quote},
	{'\"', TokenType::double_quote},
	{'>', TokenType::greater},
	{'<', TokenType::less},
	{'!', TokenType::exclamation_point},
	{'[', TokenType::open_bracket},
	{']', TokenType::close_bracket},
};

enum class BaseType {
	int_,
	char_,
	bool_,
};

struct VarType {
	BaseType base;
	bool is_array = false;
	size_t array_size = 0;

	//helpers
	//comparison

	bool operator==(const VarType& other) const
	{
		return base == other.base &&
			is_array == other.is_array &&
			array_size == other.array_size;
	}

	bool operator!=(const VarType& other) const
	{
		return !(*this == other);
	}

	//base match
	bool base_matches(const VarType& other) const
	{
		return base == other.base;
	}

	//get element type
	VarType element_type() const
	{
		return VarType{ .base = base, .is_array = false, .array_size = 0};
	}

	//create array type from this array
	VarType as_array(const size_t size) const
	{
		return VarType{ .base = base, .is_array = true, .array_size = size};
	}

	//helpers to create types easily
	static VarType make_int_lit()
	{
		return VarType{ .base = BaseType::int_, .is_array = false, .array_size = 0};
	}

	static VarType make_bool_lit()
	{
		return VarType{ .base = BaseType::bool_, .is_array = false, .array_size = 0};
	}

	static VarType make_int_array(const size_t size)
	{
		return VarType{ .base = BaseType::int_, .is_array = true, .array_size = size};
	}

	static VarType make_bool_array(const size_t size)
	{
		return VarType{ .base = BaseType::bool_, .is_array = true, .array_size = size};
	}
};

static const bidirectional_unordered_map<BaseType, TokenType> VariableBaseTypes = {
	{BaseType::int_, TokenType::int_},
	{BaseType::char_, TokenType::char_},
	{BaseType::bool_, TokenType::bool_},
};

inline std::optional<int> bin_precedence(const TokenType type) {
	switch(type) {
		case TokenType::equals_equals:
		case TokenType::not_equals:
		case TokenType::greater_equals:
		case TokenType::less_equals:
		case TokenType::greater:
		case TokenType::less:
			return 0;
		case TokenType::plus_sign:
		case TokenType::dash_sign:
			return 1;
		case TokenType::star_sign:
		case TokenType::fslash_sign:
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
				{ consume(); consume(); tokens.push_back({ .type = TokenType::equals_equals }); }
			//!=
			else if (Test_Double_SingleCharTokens(next_char, TokenType::exclamation_point, TokenType::equals))
				{ consume(); consume(); tokens.push_back({ .type = TokenType::not_equals }); }
			//>=
			else if (Test_Double_SingleCharTokens(next_char, TokenType::greater, TokenType::equals))
				{ consume(); consume(); tokens.push_back({ .type = TokenType::greater_equals }); }
			//<=
			else if (Test_Double_SingleCharTokens(next_char, TokenType::less, TokenType::equals))
				{ consume(); consume(); tokens.push_back({ .type = TokenType::less_equals }); }
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
				std::cerr << "Could not tokenize something where peek() = " << peek().value() << " and buf = " << buf << std::endl;
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

	inline bool Test_Double_SingleCharTokens(const char next_char, const TokenType type1, const TokenType type2)
	{
		return SingleCharTokens.count(next_char) &&
						 SingleCharTokens.at(next_char) == type1 &&
						 peek(1).has_value() &&
						 SingleCharTokens.count(peek(1).value()) &&
						 SingleCharTokens.at(peek(1).value()) == type2;
	}

	const std::string m_src; //m_ for members
	int m_index = 0;
};