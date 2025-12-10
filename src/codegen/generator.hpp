#pragma once

#include "ir/ir.hpp"
#include "asm_emitter.hpp"

class AsmGenerator {
public:
    explicit AsmGenerator(IRProgram prog);

    std::string generate_program();

private:
    void generate_function(const IRFunction& func);

    //helpers
    std::string to_x86_operand(const IROperand& opnd);
    std::string vreg_stack_loc(size_t vreg_id);

    const IRProgram m_prog;
    AsmEmitter m_emitter;

    size_t m_stack_size = 0;
};

#pragma region enums
enum Reg {
    rax = 1,
    rbx = 2,
    rcx = 3,
    rdx = 4,
    al = 5,
    rdi = 6,
    rsp = 7,
};

enum asm_opcodes {
    XOR,
    TEST,
    CMP,
    MOV,
    MOVZX,
    QWORD,
    SYSCALL,
};
#pragma endregion

#pragma region maps
inline std::unordered_map<Reg, std::string> regs = {
    {rax, "rax"},
    {rbx, "rbx"},
    {rcx, "rcx"},
    {rdx, "rdx"},
    {al, "al"},
    {rdi, "rdi"},
    {rsp, "rsp"},
};

inline std::unordered_map<IROpcode, std::string> op = {
    {IROpcode::ADD, "add"},
    {IROpcode::SUB, "sub"},
    {IROpcode::MUL, "imul"},
    {IROpcode::DIV, "idiv"},
    {IROpcode::GOTO, "jmp"},
    {IROpcode::GOTRUE, "jz"},
    {IROpcode::EQ, "sete"},
    {IROpcode::NEQ, "setne"},
    {IROpcode::GT, "setg"},
    {IROpcode::LT, "setl"},
    {IROpcode::GTE, "setge"},
    {IROpcode::LTE, "setle"},
};

inline std::unordered_map<asm_opcodes, std::string> aop = {
    {XOR, "xor"},
    {TEST, "test"},
    {CMP, "cmp"},
    {MOV, "mov"},
    {MOVZX, "movzx"},
    {QWORD, "QWORD"},
    {SYSCALL, "syscall"},
};
#pragma endregion

//syscalls
constexpr int EXIT_CODE = 60;
constexpr int SYS_WRITE = 1;