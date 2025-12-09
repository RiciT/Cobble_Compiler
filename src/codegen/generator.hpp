#pragma once

#include "ir/ir.hpp"

class AsmGenerator {
public:
    explicit AsmGenerator(IRProgram prog);

    std::string generate_program();

private:
    const IRProgram m_prog;
};