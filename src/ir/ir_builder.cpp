#include <cassert>
#include <ranges>

#include "ir_builder.hpp"

IRBuilder::IRBuilder(NodeProgram prog)
    : m_prog(std::move(prog))
{
}

IRProgram IRBuilder::build_ir()
{
    IRProgram program;

    IRFunction main_func_def;
    program.functions.push_back(main_func_def);
    IRFunction& main_func = program.functions.back();
    main_func.name = "_start";

    // for (const NodeStmt* stmt : m_prog.stmts)
    // {
    //     build_statement(stmt);
    // }

    IRInstruction exit;
    exit.opcode = IROpcode::EXIT;
    exit.dest = IROperand::none();
    exit.src1 = IROperand::make_lit(0);
    exit.src2 = IROperand::none();

    std::vector<IRInstruction> exit_vector;
    exit_vector.push_back(std::move(exit));

    IRBasicBlock exit_block;
    exit_block.name = "DEFAULT EXIT";
    exit_block.instructions = exit_vector;

    main_func.blocks.push_back(std::move(exit_block));

    return program;
}

//helpers
IROperand IRBuilder::create_vreg() const
{

}

std::string IRBuilder::create_label()
{

}

void IRBuilder::emit(const IRInstruction &instr) const
{

}

void IRBuilder::begin_scope() {

}

void IRBuilder::end_scope() {

}

IRBuilder::VarInfo* IRBuilder::find_var(const std::string& name) {

}

void IRBuilder::add_var(const std::string& name, VarInfo info) {

}