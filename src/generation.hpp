#pragma once

#include <algorithm>
#include <cassert>

#include "parser.hpp"

class Generator {
#pragma region public:
public:
    explicit Generator(NodeProgram prog)
        : m_prog(std::move(prog))
    {
    }

    void generate_atom(const NodeAtom* atom)
    {
        struct AtomVisitor {
            Generator& gen;
            void operator()(const NodeAtomIdent* atom_ident) const{
                const auto it = std::ranges::find_if(std::as_const(gen.m_vars), [&](const Variable& var){
                    return var.name == atom_ident->ident.value.value(); });
                if (it == gen.m_vars.cend())
                {
                    std::cerr << "Undeclared identifier: " << atom_ident->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                //moving the stackpointer
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "]";
                gen.push(offset.str());
            }
            void operator()(const NodeAtomIntLit* atom_int_lit) const {
                gen.m_output << "    mov rax, " << atom_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            }
            void operator()(const NodeAtomParen* atom_paren) const {
                gen.generate_expression(atom_paren->expr);
            }
        };
        AtomVisitor visitor({.gen = *this});
        std::visit(visitor, atom->primary_expr);
    }

    void generate_binary_expression(const NodeBinExpr* bin_expr)
    {
        struct BinExprVisitor {
            Generator& gen;
            void operator()(const NodeBinExprAdd* add) const {
                gen.generate_expression(add->rhs);
                gen.generate_expression(add->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    add rax, rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprSub* sub) const {
                gen.generate_expression(sub->rhs);
                gen.generate_expression(sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    sub rax, rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprMult* mult) const {
                gen.generate_expression(mult->rhs);
                gen.generate_expression(mult->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    mul rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprDiv* div) const {
                gen.generate_expression(div->rhs);
                gen.generate_expression(div->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    xor rdx, rdx\n";
                gen.m_output << "    div rbx\n";
                gen.push("rax");
            }
        };
        BinExprVisitor visitor { .gen = *this };
        std::visit(visitor, bin_expr->bin_expr);
    }

    void generate_expression(const NodeExpr* expr) 
    {
        struct ExprVisitor {
            Generator& gen;
            void operator()(const NodeAtom* atom) const
            {  
                gen.generate_atom(atom);
            }
            void operator()(const NodeBinExpr* bin_expr) const 
            {
                gen.generate_binary_expression(bin_expr);
            }
        };

        ExprVisitor visitor{ .gen = *this };
        std::visit(visitor, expr->expr);
    }

    void generate_scope(const NodeScope* scope)
    {
        begin_scope();

        for (const NodeStmt* stmt : scope->stmts)
        {
            generate_statement(stmt);
        }
    
        end_scope();
    }

    void generate_if_predicate(const NodeIfPredicate* pred, const std::string& end_label)
    {
        struct PredVisitor {
            Generator& gen;
            const std::string& end_label;

            void operator()(const NodeIfPredElseIf* elseif_) const
            {
                gen.m_output << "    ;; else if\n"; //comment
                gen.generate_expression(elseif_->expr);
                gen.pop("rax");
                const std::string label = gen.create_label();
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz " << label << "\n";
                gen.generate_scope(elseif_->scope);
                gen.m_output << "    jmp " << end_label << "\n";
                if (elseif_->ifpred.has_value())
                {    
                    gen.m_output << label << ":\n";
                    gen.generate_if_predicate(elseif_->ifpred.value(), end_label);
                }
            }
            void operator()(const NodeIfPredElse* else_) const
            {
                gen.m_output << "    ;; else\n"; //comment
                gen.generate_scope(else_->scope);
            }
        };

        PredVisitor visitor{ .gen = *this, .end_label = end_label };
        std::visit(visitor, pred->ifpred);
    }

    void generate_statement(const NodeStmt* stmt) 
    {
        //visitor kind of works like a Match statement so that we can decide which is it
        struct StmtVisitor {
            Generator& gen;
            void operator()(const NodeStmtExit* stmt_exit) const
            {
                gen.generate_expression(stmt_exit->expr);
                gen.m_output << "    mov rax, 60\n";
                gen.pop("rdi");
		        gen.m_output << "    syscall\n";
            }
            void operator()(const NodeStmtDef* stmt_def) const
            {
                const auto it = std::ranges::find_if(std::as_const(gen.m_vars), [&](const Variable& var){
                    return var.name == stmt_def->ident.value.value(); });
                if (it != gen.m_vars.cend()) 
                {
                    std::cerr << "Identifier already used: " << stmt_def->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                gen.m_vars.push_back({ .name = stmt_def->ident.value.value(), .stack_loc = gen.m_stack_size });
                if (stmt_def->expr)
                    gen.generate_expression(stmt_def->expr.value());
            }
            void operator()(const NodeScope* scope) const 
            {
                gen.generate_scope(scope);
            }
            void operator()(const NodeStmtIf* stmt_if) const 
            {
                gen.generate_expression(stmt_if->expr);
                gen.pop("rax");
                const std::string label = gen.create_label();
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz " << label << "\n";
                gen.generate_scope(stmt_if->scope);
                if (stmt_if->ifpred.has_value())
                {
                    const std::string end_label = gen.create_label();
                    gen.m_output << "    jmp " << end_label << "\n";
                    gen.m_output << label << ":\n";
                    gen.generate_if_predicate(stmt_if->ifpred.value(), end_label);
                    gen.m_output << end_label << ":\n";
                }
                else 
                {
                    gen.m_output << label << ":\n";
                }
            }
            void operator()(const NodeStmtAssign* stmt_assign) const 
            {
                const auto it = std::ranges::find_if(gen.m_vars, [&](const Variable& var){
                    return var.name == stmt_assign->ident.value.value();
                });
                if (it == gen.m_vars.end())
                {
                    std::cerr << "Undeclared identifier: " << stmt_assign->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen.generate_expression(stmt_assign->expr);
                gen.pop("rax");
                gen.m_output << "    mov [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], rax \n";
            }
            void operator()(const NodeStmtWhile* stmt_while) const
            {
                const std::string label = gen.create_label();
                gen.m_output << label << ":\n";

                gen.generate_expression(stmt_while->expr);
                gen.pop("rax");
                gen.m_output << "    test rax, rax\n";
                const std::string end_label = gen.create_label();
                gen.m_output << "    jz " << end_label << "\n";
                
                gen.generate_scope(stmt_while->scope);

                gen.m_output << "    jmp " << label << "\n";

                gen.m_output << end_label << ":\n";
            }
            void operator()(const NodeStmtPrint* stmt_print) const
            {
                //PRINTING INTEGERS

                gen.generate_expression(stmt_print->expr);
                gen.pop("rax");  // Number to print is now in rax

                // Convert integer to ASCII string
                gen.m_output << "    ; Convert integer in rax to ASCII\n";
                gen.m_output << "    mov rbx, 10\n";          // divisor
                gen.m_output << "    mov rcx, 0\n";           // digit counter
                gen.m_output << "    sub rsp, 32\n";          // allocate buffer on stack
                gen.m_output << "    mov rdi, rsp\n";         // rdi = buffer address
                gen.m_output << "    add rdi, 31\n";          // point to end of buffer
                gen.m_output << "    mov BYTE [rdi], 10\n";   // add newline
                gen.m_output << "    dec rdi\n";
                gen.m_output << "    inc rcx\n";

                // Handle the case where the number is 0
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jnz .convert_loop\n";
                gen.m_output << "    mov BYTE [rdi], '0'\n";
                gen.m_output << "    dec rdi\n";
                gen.m_output << "    inc rcx\n";
                gen.m_output << "    jmp .done_convert\n";

                gen.m_output << ".convert_loop:\n";
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz .done_convert\n";
                gen.m_output << "    xor rdx, rdx\n";         // clear rdx for division
                gen.m_output << "    div rbx\n";              // rax = rax/10, rdx = rax%10
                gen.m_output << "    add dl, '0'\n";          // convert digit to ASCII
                gen.m_output << "    mov [rdi], dl\n";        // store character
                gen.m_output << "    dec rdi\n";              // move buffer pointer back
                gen.m_output << "    inc rcx\n";              // increment digit count
                gen.m_output << "    jmp .convert_loop\n";

                gen.m_output << ".done_convert:\n";
                gen.m_output << "    inc rdi\n";              // adjust to first digit

                // Now print the buffer
                gen.m_output << "    mov rax, 1\n";           // sys_write
                gen.m_output << "    mov rsi, rdi\n";         // buffer address
                gen.m_output << "    mov rdi, 1\n";           // stdout
                gen.m_output << "    mov rdx, rcx\n";         // length = digit count
                gen.m_output << "    syscall\n";

                gen.m_output << "    add rsp, 32\n";          // clean up buffer
            }
            void operator()(const NodeStmtFunc* stmt_func) const
            {
                const std::string func_label = "func_" + stmt_func->ident.value.value();

                gen.m_output << func_label << ":\n";
                gen.m_output << "    push rbp\n";
                gen.m_output << "    mov rbp, rsp\n";

                if (stmt_func->params.has_value())
                {
                    //push params to two separate registers
                }
            }
            void operator()(const NodeStmtFuncCall* stmt_func_call) const
            {
                //not implemented
            }
            void operator()(const NodeStmtReturn* stmt_return) const
            {
                //not implemented
            }
        };

        StmtVisitor visitor { .gen = *this };
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
#pragma endregion

#pragma region private:
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

    void begin_scope() 
    {
        m_scopes.push_back(m_vars.size());
    }

    void end_scope()
    {
        const size_t pop_count = m_vars.size() - m_scopes.back();
        //move back stackpointer (add since the stack is upside down)
        m_output << "    add rsp, " << pop_count * 8 << "\n";
        m_stack_size -= pop_count;
        for (int i = 0; i < pop_count; i++)
        {
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    std::string create_label() 
    {
        std::stringstream strs;
        strs << "label" << m_label_count++;
        return strs.str();
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
    int m_label_count = 0;
#pragma endregion
};