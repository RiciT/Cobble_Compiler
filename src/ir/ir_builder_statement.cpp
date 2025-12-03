#include "ir_builder.hpp"

void IRBuilder::build_scope(const NodeScope* scope) {
    begin_scope();
    for (const NodeStmt* stmt : scope->stmts)
    {
        build_statement(stmt);
    }
    end_scope();
}

void IRBuilder::build_statement(const NodeStmt* stmt) {
    struct StmtVisitor {
        IRBuilder& irb;
        void operator()(const NodeStmtExit* stmt_exit) const
        {
            const auto expr_reg = irb.build_expr(stmt_exit->expr);
            irb.m_current_block->instructions.push_back({ .opcode = IROpcode::EXIT, .src1 = expr_reg});
        }
        void operator()(const NodeStmtDef* stmt_def) const
        {
            //we assume unique identifiers thanks to TypeChecker
            // assert(std::ranges::find_if(std::as_const(gen.m_vars), [&](const Variable& var){
            //     return var.name == stmt_def->ident.value.value(); }) == gen.m_vars.cend());
            //
            // if (stmt_def->type.is_array)
            // {
            //     //evaluate array size at compile time
            //     const auto size_opt = gen.evaluate_const_expr(stmt_def->array_size_expr.value());
            //
            //     assert(size_opt.has_value() && "Array size expression evaluation failed");
            //     assert(size_opt.value() > 0 && "Array size must be positive");
            //
            //     const size_t array_size = static_cast<size_t>(size_opt.value());
            //
            //     //allocate space for array
            //     const size_t total_bytes = array_size * 8;
            //     gen.m_emitter.emit("sub", "rsp", total_bytes);
            //
            //     //create type with resolved size
            //     VarType resolved_type = stmt_def->type;
            //     resolved_type.array_size = array_size;
            //
            //     gen.m_vars.push_back({
            //         .type = resolved_type,
            //         .name = std::string(stmt_def->ident.value.value()),
            //         .stack_loc = gen.m_stack_size,
            //         .is_param = false
            //     });
            //
            //     gen.m_stack_size += array_size;
            // }
            // else
            // {
            //     gen.m_vars.push_back({ .type = stmt_def->type, .name = std::string(stmt_def->ident.value.value()), .stack_loc = gen.m_stack_size });
            //     if (stmt_def->expr)
            //         gen.generate_expression(stmt_def->expr.value());
            //     else
            //     {
            //         //def initialize to 0
            //         gen.m_emitter.emit("mov", "rax", "0");
            //         gen.push("rax");
            //     }
            // }
        }
        void operator()(const NodeScope* scope) const
        {
            irb.build_scope(scope);
        }
        void operator()(const NodeStmtIf* stmt_if) const
        {
            //save register of expr
            const auto expr_reg = irb.build_expr(stmt_if->expr);
            //save label id of if label
            const auto if_in_label_id = irb.m_label_id;
            //save current block name
            const auto current_block_name = irb.m_current_block->name;
            //GOFALSE LX_enter tX
            irb.m_current_block->instructions.push_back({ IROpcode::GOTRUE,
                irb.create_label(true), expr_reg});

            //basic block of if statement
            irb.m_current_func->blocks.push_back({ .name = "if"+std::to_string(if_in_label_id), .instructions = {
                IRInstruction{ IROpcode::LABEL, irb.create_label(true, if_in_label_id) }
            } });
            irb.m_current_block = &irb.m_current_func->blocks.back();
            irb.build_scope(stmt_if->scope);
            irb.m_current_block->instructions.push_back({ IROpcode::GOTO,
                irb.create_label(false, if_in_label_id) });

            //generate predicates if there are any
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block_name;
            });
            if (stmt_if->ifpred.has_value())
            {
                irb.build_if_predicate(stmt_if->ifpred.value(), if_in_label_id);
            }
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block_name;
            });
            irb.m_current_block->instructions.push_back({ IROpcode::LABEL, irb.create_label(false, if_in_label_id)});
        }
        void operator()(const NodeStmtAssign* stmt_assign) const
        {
            // const auto it = std::ranges::find_if(gen.m_vars, [&](const Variable& var){
            //     return var.name == stmt_assign->ident.value.value(); });
            //
            // assert(it != gen.m_vars.end() && "Undeclared identifier in assignment");
            //
            // gen.generate_expression(stmt_assign->expr);
            // gen.pop("rax");
            //
            // if (it->is_param)
            // {
            //     //parameters: positive offset from rbp
            //     gen.m_emitter.emit_mov_offset("rbp", "rax", it->stack_loc);
            // }
            // else
            // {
            //     //local variables: calculated from rsp
            //     gen.m_emitter.emit_mov_offset("rsp", "rax", (gen.m_stack_size - it->stack_loc - 1) * 8);
            // }
        }
        void operator()(const NodeStmtWhile* stmt_while) const
        {
            // const std::string label = gen.create_label();
            // gen.m_emitter.emit_label(label);
            //
            // gen.generate_expression(stmt_while->expr);
            // gen.pop("rax");
            // gen.m_emitter.emit("test", "rax", "rax");
            // const std::string end_label = gen.create_label();
            // gen.m_emitter.emit("jz", end_label);
            //
            // gen.generate_scope(stmt_while->scope);
            //
            // gen.m_emitter.emit("jmp", label);
            //
            // gen.m_emitter.emit_label(end_label);
        }
        void operator()(const NodeStmtPrint* stmt_print) const
        {
            if (irb.m_current_block == nullptr)
            {
                IRBasicBlock print_block = { .name = "print", .instructions = {} };
                irb.m_current_func->blocks.push_back(print_block);
                irb.m_current_block = &irb.m_current_func->blocks.back();
            }
            const IROperand reg = irb.build_expr(stmt_print->expr);

            const IRInstruction print = { IROpcode::PRINT, reg };
            irb.m_current_block->instructions.push_back(print);
        }
        void operator()(const NodeStmtFunc* stmt_func) const
        {
            // gen.m_emitter.set_section(AsmEmitter::Section::Functions);
            //
            // const std::string func_label = "func_" + std::string(stmt_func->ident.value.value());
            //
            // gen.m_emitter.emit_label(func_label);
            // gen.m_emitter.emit("push", "rbp");
            // gen.m_emitter.emit("mov", "rbp", "rsp");
            //
            // //save the current state so function has its own scope
            // const size_t saved_stack_size = gen.m_stack_size;
            // const std::vector<Variable> saved_vars = gen.m_vars;
            // const std::vector<size_t> saved_scopes = gen.m_scopes;
            //
            // //reset for function scope
            // gen.m_stack_size = 0;
            // gen.m_vars.clear();
            // gen.m_scopes.clear();
            //
            // if (stmt_func->params.has_value())
            // {
            //     int param_index = 0;
            //     for (const auto&[type, ident] : stmt_func->params.value())
            //     {
            //         // poarameters are at [rbp + 16], [rbp + 24], etc.
            //         // +16 because: +8 for return address, +8 for saved rbp
            //         gen.m_vars.push_back({
            //             .type = type,
            //             .name = std::string(ident.value.value()),
            //             .stack_loc = static_cast<size_t>(16 + param_index * 8),
            //             .is_param = true
            //         });
            //         param_index++;
            //     }
            // }
            //
            // gen.generate_scope(stmt_func->scope);
            //
            // gen.m_emitter.emit("mov", "rax", "0");
            // gen.m_emitter.emit("mov", "rsp", "rbp");
            // gen.m_emitter.emit("pop", "rbp");
            // gen.m_emitter.emit("ret");
            //
            // //restore state
            // gen.m_stack_size = saved_stack_size;
            // gen.m_vars = saved_vars;
            // gen.m_scopes = saved_scopes;
            //
            // gen.m_emitter.set_section(AsmEmitter::Section::Main);
        }
        void operator()(const NodeStmtFuncCall* stmt_func_call) const
        {
            //push arguments right-to-left
            // if (stmt_func_call->exprs.has_value())
            // {
            //     const auto& args = stmt_func_call->exprs.value();
            //     for (int i = args.size() - 1; i >= 0; i--)
            //     {
            //         gen.generate_expression(args[i]);
            //         //expression result is already pushed
            //     }
            // }
            //
            // //push parameters onto stack so it can be popped in order
            // const std::string func_label = "func_" + std::string(stmt_func_call->ident.value.value());
            // gen.m_emitter.emit("call", func_label);
            //
            // //clean up arguments from stack
            // if (stmt_func_call->exprs.has_value())
            // {
            //     if (const size_t args_size = stmt_func_call->exprs.value().size(); args_size > 0)
            //     {
            //         gen.m_emitter.emit("add", "rsp", args_size * 8);
            //         gen.m_stack_size -= args_size;
            //     }
            // }
            //
            // // TAKE OUT THIS PART SINCE THIS MESSES WITH STACK POINTER LOCATION
            // // WILL NEED TO HANDLE EXPRESSION FUNC CALLS DIFFERENTLY
            // //return value is in rax so push it onto the stack
            // //gen.push("rax");
        }
        void operator()(const NodeStmtReturn* stmt_return) const
        {
            // if (stmt_return->expr.has_value())
            // {
            //     gen.generate_expression(stmt_return->expr.value());
            //     gen.pop("rax");
            // }
            // else
            // {
            //     // no return value so default to 0
            //     gen.m_emitter.emit("mov", "rax", "0");
            // }
            //
            // gen.m_emitter.emit("mov", "rsp", "rbp");
            // gen.m_emitter.emit("pop", "rbp");
            // gen.m_emitter.emit("ret");
        }
        void operator()(const NodeStmtArrayAssign* stmt_array_assign) const
        {
            // const auto it = std::ranges::find_if(gen.m_vars, [&](const Variable& var) {
            //     return var.name == stmt_array_assign->ident.value.value();
            // });
            //
            // assert(it != gen.m_vars.end() && "Undeclared identifier in array assignment");
            // assert(it->type.is_array && "Assigning to non-array");
            //
            // //WONT THROW AN ERROR FOR OVER INDEXING NEED TO FIX
            // //generate index expression
            // gen.generate_expression(stmt_array_assign->index);
            //
            // //generate value expression
            // gen.generate_expression(stmt_array_assign->value);
            //
            // gen.pop("rcx");  // value
            // gen.pop("rax");  // index
            //
            // //calc offset
            // gen.m_emitter.emit("mov", "rbx", "8");
            // gen.m_emitter.emit("mul", "rbx");
            //
            // //address
            // const size_t base_offset = (gen.m_stack_size - it->stack_loc - it->type.array_size) * 8;
            // gen.m_emitter.emit("mov", "rbx", "rsp");
            // gen.m_emitter.emit("add", "rbx", base_offset);
            // gen.m_emitter.emit("add", "rbx", "rax");
            //
            // //store value
            // gen.m_emitter.emit("mov", "[rbx]", "rcx");
        }
    };

    StmtVisitor visitor { .irb = *this };
    std::visit(visitor, stmt->stmt);
}

void IRBuilder::build_if_predicate(const NodeIfPredicate* pred, const size_t end_label_id) {
    struct PredVisitor {
        IRBuilder& irb;
        const size_t end_label_id;

        void operator()(const NodeIfPredElseIf* elseif_) const
        {
            //save register of expr
            const auto expr_reg = irb.build_expr(elseif_->expr);
            //save label id of if label
            const auto else_if_in_label_id = irb.m_label_id;
            //save current block name
            const auto current_block_name = irb.m_current_block->name;
            //GOFALSE LX_enter tX
            irb.m_current_block->instructions.push_back({ IROpcode::GOTRUE,
                irb.create_label(true), expr_reg});

            //basic block of elseif statement
            irb.m_current_func->blocks.push_back({ .name = "elseif"+std::to_string(else_if_in_label_id), .instructions = {
                IRInstruction{ IROpcode::LABEL, irb.create_label(true, else_if_in_label_id) }
            } });
            irb.m_current_block = &irb.m_current_func->blocks.back();
            irb.build_scope(elseif_->scope);
            irb.m_current_block->instructions.push_back({ IROpcode::GOTO,
                irb.create_label(false, end_label_id) });

            //generate predicates if there are any
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block_name;
            });
            if (elseif_->ifpred.has_value())
            {
                irb.build_if_predicate(elseif_->ifpred.value(), else_if_in_label_id);
            }
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block_name;
            });
        }
        void operator()(const NodeIfPredElse* else_) const
        {
            irb.build_scope(else_->scope);
        }
    };

    PredVisitor visitor{ .irb = *this, .end_label_id = end_label_id };
    std::visit(visitor, pred->ifpred);
}
