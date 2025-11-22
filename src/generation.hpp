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

                if (it->is_param)
                {
                    //parameters: positive offset from rbp
                    std::stringstream offset;
                    offset << "QWORD [rbp + " << it->stack_loc << "]";
                    gen.push(offset.str());
                }
                else
                {
                    //local variables: calculated from rsp
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "]";
                    gen.push(offset.str());
                }
            }
            void operator()(const NodeAtomIntLit* atom_int_lit) const {
                gen.current_stream() << "    mov rax, " << atom_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            }
            void operator()(const NodeAtomBoolLit* atom_bool_lit) const
            {
                //true = 1, false = 0
                if (atom_bool_lit->bool_lit.type == TokenType::true_)
                {
                    gen.current_stream() << "    mov rax, 1\n";
                }
                if (atom_bool_lit->bool_lit.type == TokenType::false_)
                {
                    gen.current_stream() << "    mov rax, 0\n";
                }

                gen.push("rax");
            }
            void operator()(const NodeAtomParen* atom_paren) const {
                gen.generate_expression(atom_paren->expr);
            }
            void operator()(const NodeAtomArrayAccess* atom_array_access) const
            {
                const auto it = std::ranges::find_if(gen.m_vars, [&](const Variable& var) {
                    return var.name == atom_array_access->ident.value.value();
                });

                if (it == gen.m_vars.end())
                {
                    std::cerr << "Undeclared identifier: " << atom_array_access->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (!it->type.is_array)
                {
                    std::cerr << "Cannot index non-array: " << atom_array_access->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                gen.generate_expression(atom_array_access->index);
                gen.pop("rax");

                //calc offset
                gen.current_stream() << "    mov rbx, 8\n";
                gen.current_stream() << "    mul rbx\n";

                //address
                const size_t base_offset = (gen.m_stack_size - it->stack_loc - it->type.array_size) * 8;

                gen.current_stream() << "    mov rbx, rsp\n";
                gen.current_stream() << "    add rbx, " << base_offset << "\n";
                gen.current_stream() << "    add rbx, rax\n";

                gen.current_stream() << "    mov rax, [rbx]\n";
                gen.push("rax");
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
            void operator()(const NodeBinExprEq* eq) const {
                gen.generate_expression(eq->lhs);
                gen.generate_expression(eq->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.current_stream() << "    cmp rax, rbx\n";
                gen.current_stream() << "    sete al\n";        // Set al to 1 if equal
                gen.current_stream() << "    movzx rax, al\n";  // Zero-extend to full register
                gen.push("rax");
            }
            void operator()(const NodeBinExprNotEq* neq) const {
                gen.generate_expression(neq->lhs);
                gen.generate_expression(neq->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.current_stream() << "    cmp rax, rbx\n";
                gen.current_stream() << "    setne al\n";
                gen.current_stream() << "    movzx rax, al\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprGreater* gt) const {
                gen.generate_expression(gt->lhs);
                gen.generate_expression(gt->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.current_stream() << "    cmp rax, rbx\n";
                gen.current_stream() << "    setg al\n";
                gen.current_stream() << "    movzx rax, al\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprLess* lt) const {
                gen.generate_expression(lt->lhs);
                gen.generate_expression(lt->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.current_stream() << "    cmp rax, rbx\n";
                gen.current_stream() << "    setl al\n";
                gen.current_stream() << "    movzx rax, al\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprGreaterEq* gte) const {
                gen.generate_expression(gte->lhs);
                gen.generate_expression(gte->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.current_stream() << "    cmp rax, rbx\n";
                gen.current_stream() << "    setge al\n";
                gen.current_stream() << "    movzx rax, al\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprLessEq* lte) const {
                gen.generate_expression(lte->lhs);
                gen.generate_expression(lte->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.current_stream() << "    cmp rax, rbx\n";
                gen.current_stream() << "    setle al\n";
                gen.current_stream() << "    movzx rax, al\n";
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
            void operator()(const NodeFuncCallExpr* expr_func_call) const
            {
                //push arguments right-to-left
                if (expr_func_call->exprs.has_value())
                {
                    const auto& args = expr_func_call->exprs.value();
                    for (int i = args.size() - 1; i >= 0; i--)
                    {
                        gen.generate_expression(args[i]);
                        //expression result is already pushed
                    }
                }

                //push parameters onto stack so it can be popped in order
                const std::string func_label = "func_" + expr_func_call->ident.value.value();
                gen.current_stream() << "    call " << func_label << "\n";

                //clean up arguments from stack
                if (expr_func_call->exprs.has_value())
                {
                    if (const size_t args_size = expr_func_call->exprs.value().size(); args_size > 0)
                    {
                        gen.current_stream() << "    add rsp, " << args_size * 8 << "\n";
                        gen.m_stack_size -= args_size;
                    }
                }

                gen.push("rax");
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

                if (stmt_def->type.is_array)
                {
                    //allocating space for the array
                    const size_t total_bytes = stmt_def->type.array_size * 8;
                    gen.current_stream() << "    sub rsp, " << total_bytes << "\n";

                    gen.m_vars.push_back({
                        .type = stmt_def->type,
                        .name = stmt_def->ident.value.value(),
                        .stack_loc = gen.m_stack_size,
                        .is_param = false
                    });

                    gen.m_stack_size += stmt_def->type.array_size;
                }
                else
                {
                    gen.m_vars.push_back({ .type = stmt_def->type, .name = stmt_def->ident.value.value(), .stack_loc = gen.m_stack_size });
                    if (stmt_def->expr)
                        gen.generate_expression(stmt_def->expr.value());
                    else
                    {
                        //def initialize to 0
                        gen.current_stream() << "    mov rax, 0\n";
                        gen.push("rax");
                    }
                }
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
                    return var.name == stmt_assign->ident.value.value(); });
                if (it == gen.m_vars.end())
                {
                    std::cerr << "Undeclared identifier: " << stmt_assign->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen.generate_expression(stmt_assign->expr);
                gen.pop("rax");

                if (it->is_param)
                {
                    //parameters: positive offset from rbp
                    gen.current_stream() << "    mov [rbp + " << it->stack_loc << "], rax\n";
                }
                else
                {
                    //local variables: calculated from rsp
                    gen.current_stream() << "    mov [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], rax\n";
                }
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

                //save the current state so function has its own scope
                const size_t saved_stack_size = gen.m_stack_size;
                const std::vector<Variable> saved_vars = gen.m_vars;
                const std::vector<size_t> saved_scopes = gen.m_scopes;

                //reset for function scope
                gen.m_stack_size = 0;
                gen.m_vars.clear();
                gen.m_scopes.clear();

                if (stmt_func->params.has_value())
                {
                    int param_index = 0;
                    for (const NodeFuncParam& param : stmt_func->params.value())
                    {
                        // poarameters are at [rbp + 16], [rbp + 24], etc.
                        // +16 because: +8 for return address, +8 for saved rbp
                        gen.m_vars.push_back({
                            .type = param.type,
                            .name = param.ident.value.value(),
                            .stack_loc = static_cast<size_t>(16 + param_index * 8),
                            .is_param = true
                        });
                        param_index++;
                    }
                }

                gen.generate_scope(stmt_func->scope);

                gen.current_stream() << "    mov rax, 0\n";
                gen.current_stream() << "    mov rsp, rbp\n";
                gen.current_stream() << "    pop rbp\n";
                gen.current_stream() << "    ret\n";

                //restore state
                gen.m_stack_size = saved_stack_size;
                gen.m_vars = saved_vars;
                gen.m_scopes = saved_scopes;

                gen.m_current_stream = &gen.m_output;
            }
            void operator()(const NodeStmtFuncCall* stmt_func_call) const
            {
                //push arguments right-to-left
                if (stmt_func_call->exprs.has_value())
                {
                    const auto& args = stmt_func_call->exprs.value();
                    for (int i = args.size() - 1; i >= 0; i--)
                    {
                        gen.generate_expression(args[i]);
                        //expression result is already pushed
                    }
                }

                //push parameters onto stack so it can be popped in order
                const std::string func_label = "func_" + stmt_func_call->ident.value.value();
                gen.current_stream() << "    call " << func_label << "\n";

                //clean up arguments from stack
                if (stmt_func_call->exprs.has_value())
                {
                    if (const size_t args_size = stmt_func_call->exprs.value().size(); args_size > 0)
                    {
                        gen.current_stream() << "    add rsp, " << args_size * 8 << "\n";
                        gen.m_stack_size -= args_size;
                    }
                }


                // TAKE OUT THIS PART SINCE THIS MESSES WITH STACK POINTER LOCATION
                // WILL NEED TO HANDLE EXPRESSION FUNC CALLS DIFFERENTLY
                //return value is in rax so push it onto the stack
                //gen.push("rax");
            }
            void operator()(const NodeStmtReturn* stmt_return) const
            {
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
            }
            void operator()(const NodeStmtArrayAssign* stmt_array_assign) const
            {
                const auto it = std::ranges::find_if(gen.m_vars, [&](const Variable& var) {
                    return var.name == stmt_array_assign->ident.value.value();
                });

                if (it == gen.m_vars.end())
                {
                    std::cerr << "Undeclared identifier: " << stmt_array_assign->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (!it->type.is_array)
                {
                    std::cerr << "Cannot index non-array: " << stmt_array_assign->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                //generate index expression
                gen.generate_expression(stmt_array_assign->index);

                //generate value expression
                gen.generate_expression(stmt_array_assign->value);

                gen.pop("rcx");  // value
                gen.pop("rax");  // index

                //calc offset
                gen.current_stream() << "    mov rbx, 8\n";
                gen.current_stream() << "    mul rbx\n";

                //address
                const size_t base_offset = (gen.m_stack_size - it->stack_loc - it->type.array_size) * 8;
                gen.current_stream() << "    mov rbx, rsp\n";
                gen.current_stream() << "    add rbx, " << base_offset << "\n";
                gen.current_stream() << "    add rbx, rax\n";

                //store value
                gen.current_stream() << "    mov [rbx], rcx\n";
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
        size_t slots_to_pop = 0;
        const size_t pop_count = m_vars.size() - m_scopes.back();
        //move back stackpointer (add since the stack is upside down)
        for (size_t i = 0; i < pop_count; i++)
        {
            const auto& var = m_vars[m_vars.size() - 1 - i];
            if (var.type.is_array) { slots_to_pop += var.type.array_size; }
            else { slots_to_pop += 1; }
        }

        current_stream() << "    add rsp, " << slots_to_pop * 8 << "\n";
        m_stack_size -= slots_to_pop;

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
        VarType type;
        std::string name;
        size_t stack_loc;
        bool is_param = false;
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