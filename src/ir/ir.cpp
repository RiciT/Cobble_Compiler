#include "ir.hpp"
#include  <sstream>

//helper for to string
static std::string opcode_to_string(const IROpcode op) {
    switch(op) {
        case IROpcode::ADD: return "add";
        case IROpcode::SUB: return "sub";
        case IROpcode::MUL: return "mul";
        case IROpcode::DIV: return "div";
        case IROpcode::EQ: return "=";
        case IROpcode::NEQ: return "!=";
        case IROpcode::LT: return "<";
        case IROpcode::GT: return ">";
        case IROpcode::JMP: return "jmp";
        case IROpcode::JMP_FALSE: return "jmp_false";
        case IROpcode::CALL: return "call";
        case IROpcode::COPY: return "copy";
        case IROpcode::RET: return "ret";
        case IROpcode::LOAD: return "load";
        case IROpcode::STORE: return "store";
        case IROpcode::LABEL: return ""; // Label handles itself
        case IROpcode::EXIT: return "exit";
        case IROpcode::PRINT: return "print";
        default: return "???";
    }
}

static std::string operand_to_string(const IROperand& op)
{
    switch(op.type) {
        case IROperand::IntLiteral: return std::to_string(op.val_id);
        case IROperand::VirtualReg: return "%v" + std::to_string(op.val_id);
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
    if (opcode == IROpcode::LABEL) { ss << src1.label << ":"; return ss.str(); }

    ss << tab << opcode_to_string(opcode);

    if (dest.type != IROperand::None) ss << space << operand_to_string(dest);
    if (src1.type != IROperand::None) { if (dest.type != IROperand::None) ss << comma; } ss << space << operand_to_string(src1);
    if (src2.type != IROperand::None) ss << comma << space << operand_to_string(src2);

    return ss.str();
}

void IRProgram::debug_print() const
{
    for (const auto func: functions) {
        std::cout << "FUNCTION " << func.name << ":\n";
        for (const auto block: func.blocks) {
            std::cout << "  BLOCK " << block.name << ":\n";
            for (const auto instr: block.instructions) {
                std::cout << instr.to_string() << "\n";
            }
        }
        std::cout << "\n";
    }
}
