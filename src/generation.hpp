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
        m_current_stream = &m_output;
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
                gen.current_stream() << "    mov rax, " << atom_int_lit->int_lit.value.value() << "\n";
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
                gen.current_stream() << "    add rax, rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprSub* sub) const {
                gen.generate_expression(sub->rhs);
                gen.generate_expression(sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.current_stream() << "    sub rax, rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprMult* mult) const {
                gen.generate_expression(mult->rhs);
                gen.generate_expression(mult->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.current_stream() << "    mul rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprDiv* div) const {
                gen.generate_expression(div->rhs);
                gen.generate_expression(div->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.current_stream() << "    xor rdx, rdx\n";
                gen.current_stream() << "    div rbx\n";
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
                gen.current_stream() << "    ;; else if\n"; //comment
                gen.generate_expression(elseif_->expr);
                gen.pop("rax");
                const std::string label = gen.create_label();
                gen.current_stream() << "    test rax, rax\n";
                gen.current_stream() << "    jz " << label << "\n";
                gen.generate_scope(elseif_->scope);
                gen.current_stream() << "    jmp " << end_label << "\n";
                if (elseif_->ifpred.has_value())
                {    
                    gen.current_stream() << label << ":\n";
                    gen.generate_if_predicate(elseif_->ifpred.value(), end_label);
                }
            }
            void operator()(const NodeIfPredElse* else_) const
            {
                gen.current_stream() << "    ;; else\n"; //comment
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
                gen.current_stream() << "    mov rax, 60\n";
                gen.pop("rdi");
		        gen.current_stream() << "    syscall\n";
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
                gen.current_stream() << "    test rax, rax\n";
                gen.current_stream() << "    jz " << label << "\n";
                gen.generate_scope(stmt_if->scope);
                if (stmt_if->ifpred.has_value())
                {
                    const std::string end_label = gen.create_label();
                    gen.current_stream() << "    jmp " << end_label << "\n";
                    gen.current_stream() << label << ":\n";
                    gen.generate_if_predicate(stmt_if->ifpred.value(), end_label);
                    gen.current_stream() << end_label << ":\n";
                }
                else 
                {
                    gen.current_stream() << label << ":\n";
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
                gen.current_stream() << "    mov [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], rax \n";
            }
            void operator()(const NodeStmtWhile* stmt_while) const
            {
                const std::string label = gen.create_label();
                gen.current_stream() << label << ":\n";

                gen.generate_expression(stmt_while->expr);
                gen.pop("rax");
                gen.current_stream() << "    test rax, rax\n";
                const std::string end_label = gen.create_label();
                gen.current_stream() << "    jz " << end_label << "\n";
                
                gen.generate_scope(stmt_while->scope);

                gen.current_stream() << "    jmp " << label << "\n";

                gen.current_stream() << end_label << ":\n";
            }
            void operator()(const NodeStmtPrint* stmt_print) const
            {
                //PRINTING INTEGERS

                gen.generate_expression(stmt_print->expr);
                gen.pop("rax");  // Number to print is now in rax

                // Convert integer to ASCII string
                gen.current_stream() << "    ; Convert integer in rax to ASCII\n";
                gen.current_stream() << "    mov rbx, 10\n";          // divisor
                gen.current_stream() << "    mov rcx, 0\n";           // digit counter
                gen.current_stream() << "    sub rsp, 32\n";          // allocate buffer on stack
                gen.current_stream() << "    mov rdi, rsp\n";         // rdi = buffer address
                gen.current_stream() << "    add rdi, 31\n";          // point to end of buffer
                gen.current_stream() << "    mov BYTE [rdi], 10\n";   // add newline
                gen.current_stream() << "    dec rdi\n";
                gen.current_stream() << "    inc rcx\n";

                const std::string convert_loop_label = gen.create_label();
                const std::string done_convert_label = gen.create_label();
                // Handle the case where the number is 0
                gen.current_stream() << "    test rax, rax\n";
                gen.current_stream() << "    jnz " << convert_loop_label << "\n";
                gen.current_stream() << "    mov BYTE [rdi], '0'\n";
                gen.current_stream() << "    dec rdi\n";
                gen.current_stream() << "    inc rcx\n";
                gen.current_stream() << "    jmp " << done_convert_label << "\n";

                gen.current_stream() << convert_loop_label << ":\n";
                gen.current_stream() << "    test rax, rax\n";
                gen.current_stream() << "    jz " << done_convert_label << "\n";
                gen.current_stream() << "    xor rdx, rdx\n";         // clear rdx for division
                gen.current_stream() << "    div rbx\n";              // rax = rax/10, rdx = rax%10
                gen.current_stream() << "    add dl, '0'\n";          // convert digit to ASCII
                gen.current_stream() << "    mov [rdi], dl\n";        // store character
                gen.current_stream() << "    dec rdi\n";              // move buffer pointer back
                gen.current_stream() << "    inc rcx\n";              // increment digit count
                gen.current_stream() << "    jmp " << convert_loop_label << "\n";

                gen.current_stream() << done_convert_label << ":\n";
                gen.current_stream() << "    inc rdi\n";              // adjust to first digit

                // Now print the buffer
                gen.current_stream() << "    mov rax, 1\n";           // sys_write
                gen.current_stream() << "    mov rsi, rdi\n";         // buffer address
                gen.current_stream() << "    mov rdi, 1\n";           // stdout
                gen.current_stream() << "    mov rdx, rcx\n";         // length = digit count
                gen.current_stream() << "    syscall\n";

                gen.current_stream() << "    add rsp, 32\n";          // clean up buffer
            }
            void operator()(const NodeStmtFunc* stmt_func) const
            {
                gen.m_current_stream = &gen.m_functions;

                const std::string func_label = "func_" + stmt_func->ident.value.value();

                gen.current_stream() << func_label << ":\n";
                gen.current_stream() << "    push rbp\n";
                gen.current_stream() << "    mov rbp, rsp\n";

                if (stmt_func->params.has_value())
                {
                    //push params to two separate registers
                }

                gen.generate_scope(stmt_func->scope);

                gen.current_stream() << "    mov rax, 0\n";
                gen.current_stream() << "    mov rsp, rbp\n";
                gen.current_stream() << "    pop rbp\n";
                gen.current_stream() << "    ret\n";

                gen.m_current_stream = &gen.m_output;
            }
            void operator()(const NodeStmtFuncCall* stmt_func_call) const
            {
                //push parameters onto stack so it can be popped in order
                const std::string func_label = "func_" + stmt_func_call->ident.value.value();
                gen.current_stream() << "    call " << func_label << "\n";

                //return value is in rax so push it onto the stack
                gen.current_stream() << "    push rax\n";
            }
            void operator()(const NodeStmtReturn* stmt_return) const
            {
                gen.m_current_stream = &gen.m_functions;

                if (stmt_return->expr.has_value())
                {
                    gen.generate_expression(stmt_return->expr.value());
                    gen.pop("rax");
                }
                else
                {
                    // no return value so default to 0
                    gen.current_stream() << "    mov rax, 0\n";
                }

                gen.current_stream() << "    mov rsp, rbp\n";
                gen.current_stream() << "    pop rbp\n";
                gen.current_stream() << "    ret\n";

                gen.m_current_stream = &gen.m_output;
            }
        };

        StmtVisitor visitor { .gen = *this };
        std::visit(visitor, stmt->stmt);
    }

    [[nodiscard]] std::string generate_program() 
    {
        m_current_stream = &m_output;
	    m_output << "global _start\n_start:\n";
        
        for (const NodeStmt* stmt : m_prog.stmts)
        {
            generate_statement(stmt);
        }

        //if no explicit exit, exit with 0
        m_output << "    mov rax, 60\n";
		m_output << "    mov rdi, 0\n";
		m_output << "    syscall";

        return m_output.str() + "\n\n; Function stream starting here\n" + m_functions.str() + "\n";
    }
#pragma endregion

#pragma region private:
private:

    void push(const std::string& reg)
    {
        current_stream() << "    push " << reg << "\n";
        m_stack_size++; //1 = 64bit
    }

    void pop(const std::string& reg) 
    {
        current_stream() << "    pop " << reg << "\n";
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
        current_stream() << "    add rsp, " << pop_count * 8 << "\n";
        m_stack_size -= pop_count;
        for (int i = 0; i < pop_count; i++)
        {
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    std::stringstream& current_stream() const
    {
        return *m_current_stream;
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
    std::stringstream m_functions;
    std::stringstream* m_current_stream;

    size_t m_stack_size = 0;
    std::vector<Variable> m_vars {};
    std::vector<size_t> m_scopes {};
    int m_label_count = 0;
#pragma endregion
};