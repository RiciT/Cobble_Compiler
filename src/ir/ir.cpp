#include "ir.hpp"
#include  <sstream>

//helper for opcodes in nasm
static std::string opcode_to_string(const IROpcode op) {
    switch(op) {
        case IROpcode::MOV: return "mov";
        case IROpcode::ADD: return "add";
        case IROpcode::SUB: return "sub";
        case IROpcode::MUL: return "mul";
        case IROpcode::DIV: return "div";
        case IROpcode::CMP: return "cmp";
        case IROpcode::JMP: return "jmp";
        case IROpcode::JE:  return "je";
        case IROpcode::JNE: return "jne";
        case IROpcode::JG:  return "jg";
        case IROpcode::JL:  return "jl";
        case IROpcode::JGE: return "jge";
        case IROpcode::JLE: return "jle";
        case IROpcode::CALL: return "call";
        case IROpcode::RET: return "ret";
        case IROpcode::LEA: return "lea";
        case IROpcode::LOAD: return "load";
        case IROpcode::STORE: return "store";
        case IROpcode::PUSH: return "push";
        case IROpcode::POP: return "pop";
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
    for (const auto& func : functions) {
        std::cout << "FUNCTION " << func.name << ":\n";
        for (const auto&[_, instructions] : func.blocks) {
            //std::cout << "  BLOCK " << block.name << ":\n";
            for (const auto& instr : instructions) {
                std::cout << instr.to_string() << "\n";
            }
        }
        std::cout << "\n";
    }
}
