#include <algorithm>
#include <sstream>

#include "generation.hpp"
#include "asm_emitter.hpp"

Generator::Generator(NodeProgram prog)
    : m_prog(std::move(prog))
{
    m_emitter.set_section(AsmEmitter::Section::Main);
}

[[nodiscard]] std::string Generator::generate_program()
{
    m_emitter.set_section(AsmEmitter::Section::Main);

    for (const NodeStmt* stmt : m_prog.stmts)
    {
        generate_statement(stmt);
    }

    //if no explicit exit, exit with 0
    m_emitter.emit("mov", "rax", "60");
    m_emitter.emit("mov", "rdi", "0");
    m_emitter.emit("syscall");

    return m_emitter.build_output();
}

//helpers
void Generator::push(const std::string& reg)
{
    m_emitter.emit("push", reg);
    m_stack_size++; //1 = 64bit
}

void Generator::pop(const std::string& reg)
{
    m_emitter.emit("pop", reg);
    m_stack_size--;
}

void Generator::begin_scope()
{
    m_scopes.push_back(m_vars.size());
}

void Generator::end_scope()
{
    size_t slots_to_pop = 0;
    const size_t pop_count = m_vars.size() - m_scopes.back();
    //move back stackpointer (add since the stack is upside down)
    for (size_t i = 0; i < pop_count; i++)
    {
        if (const auto& var = m_vars[m_vars.size() - 1 - i]; var.type.is_array) { slots_to_pop += var.type.array_size; }
        else { slots_to_pop += 1; }
    }

    m_emitter.emit("add", "rsp", slots_to_pop * 8);
    m_stack_size -= slots_to_pop;

    for (size_t i = 0; i < pop_count; i++)
    {
        m_vars.pop_back();
    }
    m_scopes.pop_back();
}

std::string Generator::create_label()
{
    std::stringstream strs;
    strs << "label" << m_label_count++;
    return strs.str();
}
