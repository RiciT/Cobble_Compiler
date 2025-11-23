#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "lexing/tokenization.hpp"
#include "parsing/parser.hpp"
#include "analysis/type_checker.hpp"
#include "codegen/generation.hpp"

int main(int argc, char* argv[])
{
	//Setup
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
	std::string arg2(argv[2]);

	// --- Stage 1: Tokenization ---
	Tokenizer tokenizer(std::move(contents));
	std::vector<Token> tokens = tokenizer.tokenize();

	// --- Stage 2: Parsing ---
	ErrorHandler error_handler;

	Parser parser(std::move(tokens), error_handler);
	std::optional<NodeProgram> prog = parser.parse_prog();

	// Firewall 1: If parsing failed aka returned {} (nullopt) OR logged errors
	if (!prog.has_value() || error_handler.has_errors())
	{
		std::cerr << "Parsing failed:" << std::endl;
		error_handler.dump_and_exit();
	}

	// --- Stage 3: Analysis ---
	//we can safely run the analyser here because we know that the AST is structurally valid
	TypeChecker type_checker(prog.value(), error_handler);
	type_checker.analyse_program();

	// Firewall 2: If analyser logged errors
	if (error_handler.has_errors())
	{
		std::cerr << "Analysis failed:" << std::endl;
		error_handler.dump_and_exit();
	}

	// --- Stage 4: Generation ---
	//we can safely generate now since the types are checker here
	{
		Generator generator(prog.value());
		std::fstream file(arg2+"out.asm", std::ios::out);
		file << generator.generate_program();
	}

	//creating Object file from ASM
	system(("nasm -felf64 " + arg2 + "out.asm").c_str());
	//creating executable from Object file
	system(("ld -o " + arg2 + "out " + arg2 + "out.o").c_str());

	return EXIT_SUCCESS;
}