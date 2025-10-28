#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

enum class TokenType {
    exit,
    int_lit,
    semi
};

struct Token {
    TokenType type;
    std::optional<std::string> value;
};

std::vector<Token> tokenize(const std::string& str) {
    std::vector<Token> tokens;

    std::string buf;
    for (int i = 0; i < str.length(); i++) {
        char c = str.at(i);
        if (std::isalpha(c)) {
            buf.push_back(c);
            i++;
            while (std::isalnum(str.at(i))) {
                buf.push_back(str.at(i));
                i++;
            }
            i--;

            if (buf == "exit") {
                tokens.push_back({.type = TokenType::exit});
                buf.clear();
                continue;
            }
            else {
                std::cerr << "You messed up!" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (std::isdigit(c)) {
            buf.push_back(c);
            i++;
            while (std::isdigit(str.at(i)))
            {
               buf.push_back(str.at(i));
               i++; 
            }
            i--;
            tokens.push_back({.type = TokenType::int_lit, .value = buf});
            buf.clear();
        }
        else if (c == ';') {
            tokens.push_back({.type = TokenType::semi});
        }
        else if (std::isspace(c)) {
            continue;
        }
        else {
            std::cerr << "You screwed up" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    return tokens;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cerr << "Incorrect usage. Corrrect usage: " << std::endl;
        std::cerr << "cobble <input.cb>" << std::endl;
        return EXIT_FAILURE;
    }

    
    std::string contents;
    {
        std::stringstream content_stream;
        std::fstream input(argv[1], std::ios::in);
        content_stream << input.rdbuf();
        contents = content_stream.str();
    }

    std::vector<Token> thing = tokenize(contents);

    return EXIT_SUCCESS;
} 