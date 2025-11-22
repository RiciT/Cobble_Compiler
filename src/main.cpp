#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "lexing/tokenization.hpp"
#include "parsing/parser.hpp"
#include "codegen/generation.hpp"

int main(int argc, char* argv[]) 
{
	if(argc != 3) {
		std::cerr << "Incorrect usage. Correct usage: " << std::endl;
		std::cerr << "cobble <input.cb> <output-path>" << std::endl;
		return EXIT_FAILURE;
	}
	 
	std::string contents;
	{
		std::stringstream content_stream;
		std::fstream input(argv[1], std::ios::in);
		content_stream << input.rdbuf();
		contents = content_stream.str();
	}

	Tokenizer tokenizer(std::move(contents));
	std::vector<Token> tokens = tokenizer.tokenize();

	Parser parser(std::move(tokens));
	std::optional<NodeProgram> prog = parser.parse_prog();

	if (!prog.has_value())
	{
		std::cerr << "Invalid program" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string arg2(argv[2]);

	{
		Generator generator(prog.value());
		std::fstream file(arg2+"out.asm", std::ios::out);
		file << generator.generate_program();
	}

	system(("nasm -felf64 " + arg2 + "out.asm").c_str());
	system(("ld -o " + arg2 + "out " + arg2 + "out.o").c_str());

	return EXIT_SUCCESS;
}