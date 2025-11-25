#include <cassert>
#include <ranges>

#include "ir_builder.hpp"

IRBuilder::IRBuilder(NodeProgram prog)
    : m_prog(std::move(prog))
{
}

IRProgram IRBuilder::generate_ir()
{
    IRFunction main_func;
    main_func.name = "_start"; 
    m_result.functions.push_back(main_func);
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