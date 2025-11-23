#include <cassert>
#include <algorithm>
#include <ranges>

#include "generation.hpp"

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

void Generator::generate_scope(const NodeScope* scope)
{
    begin_scope();
    for (const NodeStmt* stmt : scope->stmts)
    {
        generate_statement(stmt);
    }
    end_scope();
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
            //we assume unique identifiers thanks to TypeChecker
            assert(std::ranges::find_if(std::as_const(gen.m_vars), [&](const Variable& var){
                return var.name == stmt_def->ident.value.value(); }) == gen.m_vars.cend());

            if (stmt_def->type.is_array)
            {
                //evaluate array size at compile time
                const auto size_opt = gen.evaluate_const_expr(stmt_def->array_size_expr.value());

                assert(size_opt.has_value() && "Array size expression evaluation failed");
                assert(size_opt.value() > 0 && "Array size must be positive");

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

            assert(it != gen.m_vars.end() && "Undeclared identifier in assignment");

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

            assert(it != gen.m_vars.end() && "Undeclared identifier in array assignment");
            assert(it->type.is_array && "Assigning to non-array");

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