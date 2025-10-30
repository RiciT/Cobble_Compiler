#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./generation.hpp"

int main(int argc, char* argv[]) 
{
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

	Tokenizer tokenizer(std::move(contents));
	std::vector<Token> tokens = tokenizer.tokenize();

	Parser parser(std::move(tokens));
	std::optional<NodeProgram> prog = parser.parse_prog();

	if (!prog.has_value())
	{
		std::cerr << "Invalid program" << std::endl;
		exit(EXIT_FAILURE);
	}
	

	Generator generator(prog.value());

	{
		std::fstream file("out.asm", std::ios::out);
		file << generator.generate_program();
	}

	system("nasm -felf64 out.asm");
	system("ld -o out out.o");

	return EXIT_SUCCESS;
} 