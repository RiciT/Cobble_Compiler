#include "ir.hpp"

#include <set>
#include  <sstream>

//helper for to string
static std::string opcode_to_string(const IROpcode op) {
    switch(op) {
        case IROpcode::ADD: return "ADD";
        case IROpcode::SUB: return "SUB";
        case IROpcode::MUL: return "MUL";
        case IROpcode::DIV: return "DIV";
        case IROpcode::COPY: return "COPY";
        case IROpcode::GOTO: return "GOTO";
        case IROpcode::GOFALSE: return "GOFALSE";
        case IROpcode::ALLOC: return "ALLOC";
        case IROpcode::MOV: return "MOV";
        case IROpcode::LABEL: return ""; // Label handles itself
        case IROpcode::EXIT: return "EXIT";
        case IROpcode::PRINT: return "PRINT";
        default: return "???";
    }
}

static std::string operand_to_string(const IROperand& op)
{
    switch(op.type) {
        case IROperand::IntLiteral: return std::to_string(op.val_id);
        case IROperand::VirtualReg: return "t" + std::to_string(op.val_id);
        case IROperand::Label: return op.label;
        default: return "";
    }
}

std::string IRInstruction::to_string() const
{
    std::stringstream ss;
    const std::string tab = "    ";
    const std::string space = " ";
    const std::string comma = ",";
    if (opcode == IROpcode::LABEL) { ss << dest.label << ":"; return ss.str(); }

    if (std::set { IROpcode::ADD, IROpcode::SUB, IROpcode::MUL, IROpcode::DIV}.contains(opcode))
    {
        const char sign = opcode == IROpcode::ADD ? '+' : opcode == IROpcode::SUB ? '-' : opcode == IROpcode::MUL ? '*' : '/';
        ss << operand_to_string(dest) << " = " << operand_to_string(src1) << space << sign << space << operand_to_string(src2);
    }

    ss << tab << opcode_to_string(opcode);

    if (dest.type != IROperand::None) ss << space << operand_to_string(dest);
    if (src1.type != IROperand::None) ss << space << operand_to_string(src1);
    if (src2.type != IROperand::None) ss << comma << space << operand_to_string(src2);

    return ss.str();
}

void IRProgram::debug_print() const
{
    for (const auto [func_name, blocks, _, __]: functions) {
        for (const auto [block_name, instructions]: blocks) {
            for (const auto instr: instructions) {
                std::cout << instr.to_string() << "\n";
            }
        }
        std::cout << "\n";
    }
}
