#include "ir.hpp"

#include <set>
#include  <sstream>

//helper for to string
static std::string opcode_to_string(const IROpcode op) {
    switch(op) {
        case IROpcode::GOTO: return "GOTO";
        case IROpcode::GOTRUE: return "GOTRUE";
        case IROpcode::ALLOC: return "ALLOC";
        case IROpcode::MOV: return "MOV";
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

    if (std::set { IROpcode::ADD, IROpcode::SUB, IROpcode::MUL, IROpcode::DIV,
        IROpcode::EQ, IROpcode::NEQ, IROpcode::GT, IROpcode::LT, IROpcode::GTE, IROpcode::LTE}.contains(opcode))
    {
        const std::string sign = opcode == IROpcode::ADD ? "+" : opcode == IROpcode::SUB ? "-" : opcode == IROpcode::MUL ? "*" :
            opcode == IROpcode::DIV ? "/" : opcode == IROpcode::EQ ? "==" : opcode == IROpcode::NEQ ? "!=" :
            opcode == IROpcode::GT ? ">" : opcode == IROpcode::LT ? "<" : opcode == IROpcode::GTE ? "<=" : ">=";
        ss << tab << operand_to_string(dest) << " = " << operand_to_string(src1) << space << sign << space << operand_to_string(src2);
        return ss.str();
    }
    if (std::set { IROpcode::EQ, IROpcode::NEQ, IROpcode::GT, IROpcode::LT, IROpcode::GTE, IROpcode::LTE}.contains(opcode))
    {
        const char sign = opcode == IROpcode::ADD ? '+' : opcode == IROpcode::SUB ? '-' : opcode == IROpcode::MUL ? '*' : '/';
        ss << tab << operand_to_string(dest) << " = " << operand_to_string(src1) << space << sign << space << operand_to_string(src2);
        return ss.str();
    }
    if (opcode == IROpcode::COPY) { ss << tab << operand_to_string(dest) << " = " << operand_to_string(src1); return ss.str(); }

    ss << tab << opcode_to_string(opcode);
    if (dest.type != IROperand::None) ss << space << operand_to_string(dest);
    if (src1.type != IROperand::None) ss << space << operand_to_string(src1);
    if (src2.type != IROperand::None) ss << comma << space << operand_to_string(src2);

    return ss.str();
}

void IRProgram::debug_print() const
{
    for (const auto [func_name, blocks, _]: functions) {
        for (const auto [block_name, instructions]: blocks) {
            for (const auto instr: instructions) {
                std::cout << instr.to_string() << "\n";
            }
        }
        std::cout << "\n";
    }
}
