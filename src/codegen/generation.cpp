#include <algorithm>
#include <sstream>

#include "generation.hpp"

#include "asm_emitter.hpp"

Generator::Generator(NodeProgram prog)
    : m_prog(std::move(prog))
{
    m_emitter.set_section(AsmEmitter::Section::Main);
}

void Generator::generate_atom(const NodeAtom* atom)
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
            gen.m_emitter.emit("mov", "rax", atom_int_lit->int_lit.value.value());
            gen.push("rax");
        }
        void operator()(const NodeAtomBoolLit* atom_bool_lit) const
        {
            //true = 1, false = 0
            if (atom_bool_lit->bool_lit.type == TokenType::true_)
            {
                gen.m_emitter.emit("mov", "rax", "1");
            }
            if (atom_bool_lit->bool_lit.type == TokenType::false_)
            {
                gen.m_emitter.emit("mov", "rax", "0");
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
            gen.m_emitter.emit("mov", "rbx", "8");
            gen.m_emitter.emit("mul", "rbx");

            //address
            const size_t base_offset = (gen.m_stack_size - it->stack_loc - it->type.array_size) * 8;

            gen.m_emitter.emit("mov", "rbx", "rsp");
            gen.m_emitter.emit("add", "rbx", base_offset);
            gen.m_emitter.emit("add", "rbx", "rax");

            gen.m_emitter.emit("mov", "rax", "[rbx]");
            gen.push("rax");
        }
    };
    AtomVisitor visitor({.gen = *this});
    std::visit(visitor, atom->primary_expr);
}

void Generator::generate_binary_expression(const NodeBinExpr* bin_expr)
{
    struct BinExprVisitor {
        Generator& gen;
        void operator()(const NodeBinExprAdd* add) const {
            gen.generate_expression(add->rhs);
            gen.generate_expression(add->lhs);
            gen.pop("rax");
            gen.pop("rbx");
            gen.m_emitter.emit("add", "rax", "rbx");
            gen.push("rax");
        }
        void operator()(const NodeBinExprSub* sub) const {
            gen.generate_expression(sub->rhs);
            gen.generate_expression(sub->lhs);
            gen.pop("rax");
            gen.pop("rbx");
            gen.m_emitter.emit("sub", "rax", "rbx");
            gen.push("rax");
        }
        void operator()(const NodeBinExprMult* mult) const {
            gen.generate_expression(mult->rhs);
            gen.generate_expression(mult->lhs);
            gen.pop("rax");
            gen.pop("rbx");
            gen.m_emitter.emit("mul", "rbx");
            gen.push("rax");
        }
        void operator()(const NodeBinExprDiv* div) const {
            gen.generate_expression(div->rhs);
            gen.generate_expression(div->lhs);
            gen.pop("rax");
            gen.pop("rbx");
            gen.m_emitter.emit("xor", "rdx", "rdx");
            gen.m_emitter.emit("div", "rbx");
            gen.push("rax");
        }
        void operator()(const NodeBinExprEq* eq) const {
            gen.generate_expression(eq->lhs);
            gen.generate_expression(eq->rhs);
            gen.pop("rbx");
            gen.pop("rax");
            gen.m_emitter.emit("cmp", "rax", "rbx");
            gen.m_emitter.emit("sete", "al"); //set al to 1 if equal
            gen.m_emitter.emit("movzx", "rax", "al"); //zero-extend to full register
            gen.push("rax");
        }
        void operator()(const NodeBinExprNotEq* neq) const {
            gen.generate_expression(neq->lhs);
            gen.generate_expression(neq->rhs);
            gen.pop("rbx");
            gen.pop("rax");
            gen.m_emitter.emit("cmp", "rax", "rbx");
            gen.m_emitter.emit("setne", "al");
            gen.m_emitter.emit("movzx", "rax", "al");
            gen.push("rax");
        }
        void operator()(const NodeBinExprGreater* gt) const {
            gen.generate_expression(gt->lhs);
            gen.generate_expression(gt->rhs);
            gen.pop("rbx");
            gen.pop("rax");
            gen.m_emitter.emit("cmp", "rax", "rbx");
            gen.m_emitter.emit("setg", "al");
            gen.m_emitter.emit("movzx", "rax", "al");
            gen.push("rax");
        }
        void operator()(const NodeBinExprLess* lt) const {
            gen.generate_expression(lt->lhs);
            gen.generate_expression(lt->rhs);
            gen.pop("rbx");
            gen.pop("rax");
            gen.m_emitter.emit("cmp", "rax", "rbx");
            gen.m_emitter.emit("setl", "al");
            gen.m_emitter.emit("movzx", "rax", "al");
            gen.push("rax");
        }
        void operator()(const NodeBinExprGreaterEq* gte) const {
            gen.generate_expression(gte->lhs);
            gen.generate_expression(gte->rhs);
            gen.pop("rbx");
            gen.pop("rax");
            gen.m_emitter.emit("cmp", "rax", "rbx");
            gen.m_emitter.emit("setge", "al");
            gen.m_emitter.emit("movzx", "rax", "al");
            gen.push("rax");
        }
        void operator()(const NodeBinExprLessEq* lte) const {
            gen.generate_expression(lte->lhs);
            gen.generate_expression(lte->rhs);
            gen.pop("rbx");
            gen.pop("rax");
            gen.m_emitter.emit("cmp", "rax", "rbx");
            gen.m_emitter.emit("setle", "al");
            gen.m_emitter.emit("movzx", "rax", "al");
            gen.push("rax");
        }
    };
    BinExprVisitor visitor { .gen = *this };
    std::visit(visitor, bin_expr->bin_expr);
}

void Generator::generate_expression(const NodeExpr* expr)
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
            gen.m_emitter.emit("call", func_label);

            //clean up arguments from stack
            if (expr_func_call->exprs.has_value())
            {
                if (const size_t args_size = expr_func_call->exprs.value().size(); args_size > 0)
                {
                    gen.m_emitter.emit("add", "rsp", args_size * 8);
                    gen.m_stack_size -= args_size;
                }
            }
            gen.push("rax");
        }
    };

    ExprVisitor visitor{ .gen = *this };
    std::visit(visitor, expr->expr);
}

void Generator::generate_scope(const NodeScope* scope)
{
    begin_scope();

    for (const NodeStmt* stmt : scope->stmts)
    {
        generate_statement(stmt);
    }

    end_scope();
}

void Generator::generate_if_predicate(const NodeIfPredicate* pred, const std::string& end_label)
{
    struct PredVisitor {
        Generator& gen;
        const std::string& end_label;

        void operator()(const NodeIfPredElseIf* elseif_) const
        {
            gen.m_emitter.emit_comment("else if");
            gen.generate_expression(elseif_->expr);
            gen.pop("rax");
            const std::string label = gen.create_label();
            gen.m_emitter.emit("test", "rax", "rax");
            gen.m_emitter.emit("jz", label);
            gen.generate_scope(elseif_->scope);
            gen.m_emitter.emit("jmp", end_label);
            if (elseif_->ifpred.has_value())
            {
                gen.m_emitter.emit_label(label);
                gen.generate_if_predicate(elseif_->ifpred.value(), end_label);
            }
        }
        void operator()(const NodeIfPredElse* else_) const
        {
            gen.m_emitter.emit_comment("else");
            gen.generate_scope(else_->scope);
        }
    };

    PredVisitor visitor{ .gen = *this, .end_label = end_label };
    std::visit(visitor, pred->ifpred);
}

void Generator::generate_statement(const NodeStmt* stmt)
{
    //visitor kind of works like a Match statement so that we can decide which is it
    struct StmtVisitor {
        Generator& gen;
        void operator()(const NodeStmtExit* stmt_exit) const
        {
            gen.generate_expression(stmt_exit->expr);
            gen.m_emitter.emit("mov", "rax", "60");
            gen.pop("rdi");
            gen.m_emitter.emit("syscall");
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
                //evaluate array size at compile time
                const auto size_opt = gen.evaluate_const_expr(stmt_def->array_size_expr.value());
                if (!size_opt.has_value())
                {
                    std::cerr << "Array size must be a compile-time constant expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (size_opt.value() <= 0)
                {
                    std::cerr << "Array size must be positive" << std::endl;
                    exit(EXIT_FAILURE);
                }

                const size_t array_size = static_cast<size_t>(size_opt.value());

                //allocate space for array
                const size_t total_bytes = array_size * 8;
                gen.m_emitter.emit("sub", "rsp", total_bytes);

                //create type with resolved size
                VarType resolved_type = stmt_def->type;
                resolved_type.array_size = array_size;

                gen.m_vars.push_back({
                    .type = resolved_type,
                    .name = stmt_def->ident.value.value(),
                    .stack_loc = gen.m_stack_size,
                    .is_param = false
                });

                gen.m_stack_size += array_size;
            }
            else
            {
                gen.m_vars.push_back({ .type = stmt_def->type, .name = stmt_def->ident.value.value(), .stack_loc = gen.m_stack_size });
                if (stmt_def->expr)
                    gen.generate_expression(stmt_def->expr.value());
                else
                {
                    //def initialize to 0
                    gen.m_emitter.emit("mov", "rax", "0");
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
            gen.m_emitter.emit("test", "rax", "rax");
            gen.m_emitter.emit("jz", label);
            gen.generate_scope(stmt_if->scope);
            if (stmt_if->ifpred.has_value())
            {
                const std::string end_label = gen.create_label();
                gen.m_emitter.emit("jmp", end_label);
                gen.m_emitter.emit_label(label);
                gen.generate_if_predicate(stmt_if->ifpred.value(), end_label);
                gen.m_emitter.emit_label(end_label);
            }
            else
            {
                gen.m_emitter.emit_label(label);
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
                gen.m_emitter.emit_mov_offset("rbp", "rax", it->stack_loc);
            }
            else
            {
                //local variables: calculated from rsp
                gen.m_emitter.emit_mov_offset("rsp", "rax", (gen.m_stack_size - it->stack_loc - 1) * 8);
            }
        }
        void operator()(const NodeStmtWhile* stmt_while) const
        {
            const std::string label = gen.create_label();
            gen.m_emitter.emit_label(label);

            gen.generate_expression(stmt_while->expr);
            gen.pop("rax");
            gen.m_emitter.emit("test", "rax", "rax");
            const std::string end_label = gen.create_label();
            gen.m_emitter.emit("jz", end_label);

            gen.generate_scope(stmt_while->scope);

            gen.m_emitter.emit("jmp", label);

            gen.m_emitter.emit_label(end_label);
        }
        void operator()(const NodeStmtPrint* stmt_print) const
        {
            //PRINTING INTEGERS

            gen.generate_expression(stmt_print->expr);
            gen.pop("rax");  // Number to print is now in rax

            //convert integer to ASCII string
            gen.m_emitter.emit_comment("Convert integer in rax to ASCII");
            gen.m_emitter.emit("mov", "rbx", "10");
            gen.m_emitter.emit("mov", "rcx", "0");
            gen.m_emitter.emit("sub", "rsp", "32");
            gen.m_emitter.emit("mov", "rdi", "rsp");
            gen.m_emitter.emit("add", "rdi", "31"); //point to end of buffer
            gen.m_emitter.emit("mov", "BYTE [rdi]", "10"); //add newline
            gen.m_emitter.emit("dec", "rdi");
            gen.m_emitter.emit("inc", "rcx");

            const std::string convert_loop_label = gen.create_label();
            const std::string done_convert_label = gen.create_label();
            //handle the case where the number is 0
            gen.m_emitter.emit("test", "rax", "rax");
            gen.m_emitter.emit("jnz", convert_loop_label);
            gen.m_emitter.emit("mov", "BYTE [rdi]", "'0'");
            gen.m_emitter.emit("dec", "rdi");
            gen.m_emitter.emit("inc", "rcx");
            gen.m_emitter.emit("jmp", done_convert_label);

            gen.m_emitter.emit_label(convert_loop_label);
            gen.m_emitter.emit("test", "rax", "rax");
            gen.m_emitter.emit("jz", done_convert_label);
            gen.m_emitter.emit("xor", "rdx", "rdx");      //clear rdx for division
            gen.m_emitter.emit("div", "rbx");             //rax = rax/10, rdx = rax%10
            gen.m_emitter.emit("add", "dl", "'0'");       //convert digit to ASCII
            gen.m_emitter.emit("mov", "[rdi]", "dl");     //store character
            gen.m_emitter.emit("dec", "rdi");             //move buffer pointer back
            gen.m_emitter.emit("inc", "rcx");             //increment digit count
            gen.m_emitter.emit("jmp", convert_loop_label);

            gen.m_emitter.emit_label(done_convert_label);
            gen.m_emitter.emit("inc", "rdi");             //adjust to first digit

            //now print the buffer
            gen.m_emitter.emit("mov", "rax", "1");        //sys_write
            gen.m_emitter.emit("mov", "rsi", "rdi");      //buffer address
            gen.m_emitter.emit("mov", "rdi", "1");        //stdout
            gen.m_emitter.emit("mov", "rdx", "rcx");      //length = digit count
            gen.m_emitter.emit("syscall");

            gen.m_emitter.emit("add", "rsp", "32");       //clean up buffer
        }
        void operator()(const NodeStmtFunc* stmt_func) const
        {
            gen.m_emitter.set_section(AsmEmitter::Section::Functions);

            const std::string func_label = "func_" + stmt_func->ident.value.value();

            gen.m_emitter.emit_label(func_label);
            gen.m_emitter.emit("push", "rbp");
            gen.m_emitter.emit("mov", "rbp", "rsp");

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
                for (const auto&[type, ident] : stmt_func->params.value())
                {
                    // poarameters are at [rbp + 16], [rbp + 24], etc.
                    // +16 because: +8 for return address, +8 for saved rbp
                    gen.m_vars.push_back({
                        .type = type,
                        .name = ident.value.value(),
                        .stack_loc = static_cast<size_t>(16 + param_index * 8),
                        .is_param = true
                    });
                    param_index++;
                }
            }

            gen.generate_scope(stmt_func->scope);

            gen.m_emitter.emit("mov", "rax", "0");
            gen.m_emitter.emit("mov", "rsp", "rbp");
            gen.m_emitter.emit("pop", "rbp");
            gen.m_emitter.emit("ret");

            //restore state
            gen.m_stack_size = saved_stack_size;
            gen.m_vars = saved_vars;
            gen.m_scopes = saved_scopes;

            gen.m_emitter.set_section(AsmEmitter::Section::Main);
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
            gen.m_emitter.emit("call", func_label);

            //clean up arguments from stack
            if (stmt_func_call->exprs.has_value())
            {
                if (const size_t args_size = stmt_func_call->exprs.value().size(); args_size > 0)
                {
                    gen.m_emitter.emit("add", "rsp", args_size * 8);
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
                gen.m_emitter.emit("mov", "rax", "0");
            }

            gen.m_emitter.emit("mov", "rsp", "rbp");
            gen.m_emitter.emit("pop", "rbp");
            gen.m_emitter.emit("ret");
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

            //WONT THROW AN ERROR FOR OVER INDEXING NEED TO FIX
            //generate index expression
            gen.generate_expression(stmt_array_assign->index);

            //generate value expression
            gen.generate_expression(stmt_array_assign->value);

            gen.pop("rcx");  // value
            gen.pop("rax");  // index

            //calc offset
            gen.m_emitter.emit("mov", "rbx", "8");
            gen.m_emitter.emit("mul", "rbx");

            //address
            const size_t base_offset = (gen.m_stack_size - it->stack_loc - it->type.array_size) * 8;
            gen.m_emitter.emit("mov", "rbx", "rsp");
            gen.m_emitter.emit("add", "rbx", base_offset);
            gen.m_emitter.emit("add", "rbx", "rax");

            //store value
            gen.m_emitter.emit("mov", "[rbx]", "rcx");
        }
    };

    StmtVisitor visitor { .gen = *this };
    std::visit(visitor, stmt->stmt);
}


//temporary helpers for evaluating const expression
std::optional<int64_t> Generator::evaluate_const_expr(const NodeExpr* expr)
{
    struct ConstExprVisitor {
        Generator& gen;

        std::optional<int64_t> operator()(const NodeAtom* atom) const
        {
            return gen.evaluate_const_atom(atom);
        }

        std::optional<int64_t> operator()(const NodeBinExpr* bin_expr) const
        {
            return gen.evaluate_const_binexpr(bin_expr);
        }

        std::optional<int64_t> operator()(const NodeFuncCallExpr*) const
        {
            return {};  // Function calls cannot be evaluated at compile time
        }
    };

    ConstExprVisitor visitor{.gen = *this};
    return std::visit(visitor, expr->expr);
}

std::optional<int64_t> Generator::evaluate_const_atom(const NodeAtom* atom)
{
    struct AtomVisitor {
        Generator& gen;

        std::optional<int64_t> operator()(const NodeAtomIntLit* int_lit) const
        {
            return std::stoll(int_lit->int_lit.value.value());
        }

        std::optional<int64_t> operator()(const NodeAtomIdent*) const
        {
            return {};  // Variables cannot be evaluated at compile time
        }

        std::optional<int64_t> operator()(const NodeAtomParen* paren) const
        {
            return gen.evaluate_const_expr(paren->expr);
        }

        std::optional<int64_t> operator()(const NodeAtomBoolLit* bool_lit) const
        {
            return bool_lit->bool_lit.type == TokenType::true_ ? 1 : 0;
        }

        std::optional<int64_t> operator()(const NodeAtomArrayAccess*) const
        {
            return {};  // Array access cannot be evaluated at compile time
        }
    };

    AtomVisitor visitor{.gen = *this};
    return std::visit(visitor, atom->primary_expr);
}

std::optional<int64_t> Generator::evaluate_const_binexpr(const NodeBinExpr* bin_expr)
{
    struct BinExprVisitor {
        Generator& gen;

        std::optional<int64_t> operator()(const NodeBinExprAdd* add) const
        {
            const auto lhs = gen.evaluate_const_expr(add->lhs);
            if (const auto rhs = gen.evaluate_const_expr(add->rhs); lhs && rhs) return *lhs + *rhs;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprSub* sub) const
        {
            const auto lhs = gen.evaluate_const_expr(sub->lhs);
            if (const auto rhs = gen.evaluate_const_expr(sub->rhs); lhs && rhs) return *lhs - *rhs;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprMult* mult) const
        {
            const auto lhs = gen.evaluate_const_expr(mult->lhs);
            if (const auto rhs = gen.evaluate_const_expr(mult->rhs); lhs && rhs) return *lhs * *rhs;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprDiv* div) const
        {
            const auto lhs = gen.evaluate_const_expr(div->lhs);
            if (const auto rhs = gen.evaluate_const_expr(div->rhs); lhs && rhs && *rhs != 0) return *lhs / *rhs;
            return {};
        }

        // Add comparison operators if you have them
        std::optional<int64_t> operator()(const NodeBinExprEq* eq) const
        {
            const auto lhs = gen.evaluate_const_expr(eq->lhs);
            if (const auto rhs = gen.evaluate_const_expr(eq->rhs); lhs && rhs) return (*lhs == *rhs) ? 1 : 0;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprNotEq* neq) const
        {
            const auto lhs = gen.evaluate_const_expr(neq->lhs);
            if (const auto rhs = gen.evaluate_const_expr(neq->rhs); lhs && rhs) return (*lhs != *rhs) ? 1 : 0;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprLess* lt) const
        {
            const auto lhs = gen.evaluate_const_expr(lt->lhs);
            if (const auto rhs = gen.evaluate_const_expr(lt->rhs); lhs && rhs) return (*lhs < *rhs) ? 1 : 0;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprGreater* gt) const
        {
            const auto lhs = gen.evaluate_const_expr(gt->lhs);
            if (const auto rhs = gen.evaluate_const_expr(gt->rhs); lhs && rhs) return (*lhs > *rhs) ? 1 : 0;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprLessEq* lte) const
        {
            const auto lhs = gen.evaluate_const_expr(lte->lhs);
            if (const auto rhs = gen.evaluate_const_expr(lte->rhs); lhs && rhs) return (*lhs <= *rhs) ? 1 : 0;
            return {};
        }

        std::optional<int64_t> operator()(const NodeBinExprGreaterEq* gte) const
        {
            const auto lhs = gen.evaluate_const_expr(gte->lhs);
            if (const auto rhs = gen.evaluate_const_expr(gte->rhs); lhs && rhs) return (*lhs >= *rhs) ? 1 : 0;
            return {};
        }
    };

    BinExprVisitor visitor{.gen = *this};
    return std::visit(visitor, bin_expr->bin_expr);
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
