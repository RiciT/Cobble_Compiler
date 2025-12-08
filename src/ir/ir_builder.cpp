#include <ranges>

#include "ir_builder.hpp"

IRBuilder::IRBuilder(NodeProgram prog)
    : m_prog(std::move(prog))
{
}

IRProgram IRBuilder::build_ir()
{
    IRProgram program;
    // ReSharper disable once CppDFALocalValueEscapesFunction
    m_current_program = &program; //the address is safe as everything is called from build_ir()

    const IRInstruction funcstart = { IROpcode::LABEL, { .type = IROperand::Type::Label, .label = "_start" } };
    const IRFunction main_func_def = { .name = "MAIN", .blocks = { IRBasicBlock{ .name = "def_enter", .instructions = std::vector{funcstart}} } };
    m_current_program->functions.push_back(main_func_def);
    m_current_func = &m_current_program->functions.front();
    m_current_block = &m_current_func->blocks.front();

    for (const NodeStmt* stmt : m_prog.stmts)
    {
        build_statement(stmt, 0);
    }

    m_current_block->instructions.push_back({ .opcode = IROpcode::EXIT, .src1 = IROperand::make_lit(0)});
    return program;
}

//helpers
IROperand IRBuilder::create_vreg() const
{
    return IROperand::make_reg(m_vreg_count++);
}
IROperand IRBuilder::create_label(const bool isEnter, const std::string &name)
{
    return IROperand::make_label(isEnter, 0, name);
}
IROperand IRBuilder::create_label(const bool isEnter, const size_t id)
{
    return IROperand::make_label(isEnter, id, "");
}
IROperand IRBuilder::create_label(const bool isEnter) const
{
    return IROperand::make_label(isEnter, m_label_id++, "");
}
void IRBuilder::begin_scope()
{
    m_scopes.push_back(m_vars.size());
}
void IRBuilder::end_scope()
{
    size_t slots_to_pop = 0;
    const size_t pop_count = m_vars.size() - m_scopes.back();
    //move back stackpointer (add since the stack is upside down)
    for (size_t i = 0; i < pop_count; i++)
    {
        if (const auto& var = m_vars[m_vars.size() - 1 - i]; var.type.is_array) { slots_to_pop += var.type.array_size; }
        else { slots_to_pop += 1; }
    }

    for (size_t i = 0; i < pop_count; i++)
    {
        m_vars.pop_back();
    }
    m_scopes.pop_back();
}
