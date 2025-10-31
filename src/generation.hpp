#pragma once

#include <algorithm>
#include <cassert>
#include <unordered_map>

#include "parser.hpp"

class Generator {
public:
    inline Generator(NodeProgram prog)
        : m_prog(std::move(prog))
    {
    }

    void generate_atom(const NodeAtom* atom) 
    {
        struct AtomVisitor {
            Generator* gen;
            void operator()(const NodeAtomIdent* atom_ident) const{
                if (!gen->m_vars.contains(atom_ident->ident.value.value()))
                {
                    std::cerr << "Undeclared identifier: " << atom_ident->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                //moving the stackpointer
                const auto& var = gen->m_vars.at(atom_ident->ident.value.value());
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]\n";
                gen->push(offset.str());
            }
            void operator()(const NodeAtomIntLit* atom_int_lit) const {
                gen->m_output << "    mov rax, " << atom_int_lit->int_lit.value.value() << "\n";
                gen->push("rax");
            }
        };
        AtomVisitor visitor({.gen = this});
        std::visit(visitor, atom->primary_expr);
    }

    void generate_expression(const NodeExpr* expr) 
    {
        struct ExprVisitor {
            Generator* gen;
            void operator()(const NodeAtom* atom) const
            {  
                gen->generate_atom(atom);
            }
            void operator()(const NodeBinExpr* bin_expr) const 
            {
                gen->generate_expression(bin_expr->bin_expr->lhs);
                gen->generate_expression(bin_expr->bin_expr->rhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    add rax, rbx\n";
                gen->push("rax");
            }
        };

        ExprVisitor visitor{ .gen = this };
        std::visit(visitor, expr->expr);
    }

    void generate_statement(const NodeStmt* stmt) 
    {
        //visitor kind of works like a Match statement so that we can decide which is it
        struct StmtVisitor {
            Generator* gen;
            void operator()(const NodeStmtExit* stmt_exit) const
            {
                gen->generate_expression(stmt_exit->expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
		        gen->m_output << "    syscall\n";
            }
            void operator()(const NodeStmtDef* stmt_def) const
            {
                if (gen->m_vars.contains(stmt_def->ident.value.value())) 
                {
                    std::cerr << "Identifier already used: " << stmt_def->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                gen->m_vars.insert({stmt_def->ident.value.value(), Variable {.stack_loc = gen->m_stack_size }});
                gen->generate_expression(stmt_def->expr);
            }
        };

        StmtVisitor visitor { .gen = this };
        std::visit(visitor, stmt->stmt);
    }

    [[nodiscard]] std::string generate_program() 
    {
	    m_output << "global _start\n_start:\n";
        
        for (const NodeStmt* stmt : m_prog.stmts)
        {
            generate_statement(stmt);
        }
        

        //if no explicit exit, exit with 0
        m_output << "    mov rax, 60\n";
		m_output << "    mov rdi, 0\n";
		m_output << "    syscall";
        return m_output.str();
    }

private:

    void push(const std::string& reg)
    {
        m_output << "    push " << reg << "\n";
        m_stack_size++; //1 = 64bit
    }

    void pop(const std::string& reg) 
    {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    struct Variable 
    {
        size_t stack_loc;
    };

    const NodeProgram m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    std::unordered_map<std::string, Variable> m_vars {};
};