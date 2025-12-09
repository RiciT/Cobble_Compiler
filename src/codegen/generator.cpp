#include "generator.hpp"

AsmGenerator::AsmGenerator(IRProgram prog)
    : m_prog(std::move(prog))
{
}

std::string AsmGenerator::generate_program()
{
    return "";
}

