#pragma once

#include <algorithm>
#include <cassert>

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
                auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Variable& var){
                    return var.name == atom_ident->ident.value.value(); });
                if (it == gen->m_vars.cend())
                {
                    std::cerr << "Undeclared identifier: " << atom_ident->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                //moving the stackpointer
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen->m_stack_size - (*it).stack_loc - 1) * 8 << "]\n";
                gen->push(offset.str());
            }
            void operator()(const NodeAtomIntLit* atom_int_lit) const {
                gen->m_output << "    mov rax, " << atom_int_lit->int_lit.value.value() << "\n";
                gen->push("rax");
            }
            void operator()(const NodeAtomParen* atom_paren) const {
                gen->generate_expression(atom_paren->expr);
            }
        };
        AtomVisitor visitor({.gen = this});
        std::visit(visitor, atom->primary_expr);
    }

    void generate_binary_expression(const NodeBinExpr* bin_expr)
    {
        struct BinExprVisitor {
            Generator* gen;
            void operator()(const NodeBinExprAdd* add) const {
                gen->generate_expression(add->rhs);
                gen->generate_expression(add->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    add rax, rbx\n";
                gen->push("rax");
            }
            void operator()(const NodeBinExprSub* sub) const {
                gen->generate_expression(sub->rhs);
                gen->generate_expression(sub->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    sub rax, rbx\n";
                gen->push("rax");
            }
            void operator()(const NodeBinExprMult* mult) const {
                gen->generate_expression(mult->rhs);
                gen->generate_expression(mult->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    mul rbx\n";
                gen->push("rax");
            }
            void operator()(const NodeBinExprDiv* div) const {
                gen->generate_expression(div->rhs);
                gen->generate_expression(div->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    xor rdx, rdx\n";
                gen->m_output << "    div rbx\n";
                gen->push("rax");
            }
        };
        BinExprVisitor visitor { .gen = this };
        std::visit(visitor, bin_expr->bin_expr);
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
                gen->generate_binary_expression(bin_expr);
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
                auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Variable& var){
                    return var.name == stmt_def->ident.value.value(); });
                if (it != gen->m_vars.cend()) 
                {
                    std::cerr << "Identifier already used: " << stmt_def->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                gen->m_vars.push_back({ .name = stmt_def->ident.value.value(), .stack_loc = gen->m_stack_size });
                gen->generate_expression(stmt_def->expr);
            }
            void operator()(const NodeStmtScope* scope) const 
            {
                
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
        std::string name;
        size_t stack_loc;
    };

    const NodeProgram m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    std::vector<Variable> m_vars {};
    std::vector<size_t> m_scopes {};
};