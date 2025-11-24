#pragma once

#include <string>
#include <vector>
#include <iostream>

//operands
struct IROperand {
    enum Type {
        None,
        IntLiteral, //Raw numbers
        VirtualReg, //%0, %1 etc.. - infinite registers
        Label,      //strings for control flow
    } type = None; //i just learned you can do this and find it very cool
    size_t val_id = 0;  //integer or vreg id
    std::string label;

    static IROperand make_reg(const size_t id) { IROperand op; op.type = VirtualReg; op.val_id = id; return op; }
    static IROperand make_lit(const size_t val) { IROperand op; op.type = IntLiteral; op.val_id = val; return op; }
    static IROperand make_label(const std::string& label) { IROperand op; op.type = Label; op.label = std::move(label); return op; }
    static IROperand none() { return {}; }

    //helpers for equality
    bool operator==(const IROperand& other) const
    {
        if (type != other.type) return false;
        if (type == Label) return label == other.label;
        return val_id == other.val_id;
    }
};

//opcodes
enum class IROpcode {
    //mov
    MOV,
    //arithmetic
    ADD, SUB, MUL, DIV,
    //comparison
    CMP,
    //control flow
    JMP,
    JE,
    JNE,
    JG,
    JL,
    JGE,
    JLE,
    //functions
    CALL,
    RET,
    //memory / pointers
    LEA,
    LOAD,
    STORE,
    PUSH,
    POP,
    //meta
    LABEL,
    EXIT,
    PRINT
};

//instructions
struct IRInstruction {
    IROpcode opcode;
    IROperand dest;
    IROperand src1;
    IROperand src2;

    std::string to_string() const;

    //helpers for cleaner code
    static IRInstruction make_mov(const IROperand &dest, const IROperand &src) {
        return {IROpcode::MOV, dest, src, IROperand::none()};
    }
    static IRInstruction make_binary(const IROpcode op, const IROperand &dest, const IROperand &lhs, const IROperand &rhs) {
        return {op, dest, lhs, rhs};
    }
    static IRInstruction make_cmp(const IROperand &lhs, const IROperand &rhs) {
        return {IROpcode::CMP, IROperand::none(), lhs, rhs};
    }
    static IRInstruction make_jump(const IROpcode op, const std::string &label) {
        return {op, IROperand::none(), IROperand::make_label(label), IROperand::none()};
    }
    static IRInstruction make_label(const std::string &label) {
        return {IROpcode::LABEL, IROperand::none(), IROperand::make_label(label), IROperand::none()};
    }
    static IRInstruction make_ret(const IROperand &val) {
        return {IROpcode::RET, IROperand::none(), val, IROperand::none()};
    }
};

//structure
struct IRBasicBlock {
    std::string name;
    std::vector<IRInstruction> instructions;
};

struct IRFunction {
    std::string name;
    std::vector <IRBasicBlock> blocks;

    //metadata for allocator
    size_t vreg_count = 0;
    std::vector<IROperand> params; //list of vregs for params
};

struct IRProgram {
    std::vector<IRFunction> functions;

    void debug_print() const;
};