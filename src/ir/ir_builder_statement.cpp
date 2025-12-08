#include <cassert>

#include "ir_builder.hpp"

void IRBuilder::build_statement(const NodeStmt* stmt, const int parent_block_index) {
    struct StmtVisitor {
        IRBuilder& irb;
        int parent_block_index;
        void operator()(const NodeStmtExit* stmt_exit) const
        {
            const auto expr_reg = irb.build_expr(stmt_exit->expr);
            irb.m_current_block->instructions.push_back({ .opcode = IROpcode::EXIT, .src1 = expr_reg});
        }
        void operator()(const NodeStmtDef* stmt_def) const
        {
            //we assume unique identifiers thanks to TypeChecker
            assert(std::ranges::find_if(std::as_const(irb.m_vars), [&](const VarInfo& var){
                return var.name == stmt_def->ident.value.value(); }) == irb.m_vars.cend());

            if (stmt_def->type.is_array)
            {
                //do smth
            }
            else
            {
                const IROperand reg = irb.create_vreg();
                IROperand expr;
                if (stmt_def->expr)
                    expr = irb.build_expr(stmt_def->expr.value());
                else
                    expr = IROperand::make_lit(0); //def init to 0

                irb.m_current_block->instructions.push_back({ IROpcode::COPY, reg, expr });
                irb.m_vars.push_back({ .reg = reg, .type = stmt_def->type, .name = std::string(stmt_def->ident.value.value()) });
            }
        } //do arrays
        void operator()(const NodeScope* scope) const
        {
            irb.build_scope(scope, parent_block_index);
        }
        void operator()(const NodeStmtIf* stmt_if) const
        {
            //save register of expr
            const auto expr_reg = irb.build_expr(stmt_if->expr);
            //save label id of if label
            const auto if_in_label_id = irb.m_label_id;
            //save current block name
            const auto current_block_name = irb.m_current_block->name;
            //GOTRUE LX_enter tX
            irb.m_current_block->instructions.push_back({ IROpcode::GOTRUE,
                irb.create_label(true), expr_reg});

            //basic block of if statement
            irb.m_current_block = &*irb.m_current_func->blocks.insert(
                irb.m_current_func->blocks.begin() + parent_block_index + 1,
                { .name = "if"+std::to_string(if_in_label_id), .instructions = {
                    IRInstruction{ IROpcode::LABEL, irb.create_label(true, if_in_label_id) }
            } });
            irb.build_scope(stmt_if->scope, parent_block_index + 1);
            irb.m_current_block->instructions.push_back({ IROpcode::GOTO,
                irb.create_label(false, if_in_label_id) });

            //generate predicates if there are any
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block_name;
            });
            if (stmt_if->ifpred.has_value())
            {
                irb.build_if_predicate(stmt_if->ifpred.value(), if_in_label_id, parent_block_index);
            }

            //create new block at the after the current "main" block with the ifexit label
            //and immediatly change to it as the current block
            irb.m_current_block = &*irb.m_current_func->blocks.insert(irb.m_current_func->blocks.begin() + parent_block_index + 1,
                { .name = "if_exit" + std::to_string(if_in_label_id), .instructions = {
                    { IROpcode::LABEL, irb.create_label(false, if_in_label_id)}
                } });
            if (parent_block_index <= irb.m_current_func->main_control_flow_index) irb.m_current_func->main_control_flow_index++;
        }
        void operator()(const NodeStmtAssign* stmt_assign) const
        {
            const auto it = std::ranges::find_if(irb.m_vars, [&](const VarInfo& var){
                return var.name == stmt_assign->ident.value.value(); });

            assert(it != irb.m_vars.end() && "Undeclared identifier in assignment");

            const auto reg = irb.build_expr(stmt_assign->expr);
            irb.m_current_block->instructions.push_back({IROpcode::COPY, it->reg, reg });
        }
        void operator()(const NodeStmtWhile* stmt_while) const
        {
            auto const while_label_id = irb.m_label_id;
            if (parent_block_index <= irb.m_current_func->main_control_flow_index) irb.m_current_func->main_control_flow_index++;
            irb.m_current_block = &*irb.m_current_func->blocks.insert(irb.m_current_func->blocks.begin() + parent_block_index + 1,
                                        { .name = "while_enter" + std::to_string(while_label_id), .instructions = {} });
            irb.m_current_block->instructions.push_back({ IROpcode::LABEL,
                irb.create_label(false)});
            auto const expr_reg = irb.build_expr(stmt_while->expr);
            irb.m_current_block->instructions.push_back({ IROpcode::GOTRUE,
                irb.create_label(true, while_label_id), expr_reg});

            //basic block
            irb.m_current_block = &*irb.m_current_func->blocks.insert(irb.m_current_func->blocks.begin() + parent_block_index + 2,
                { .name = "while"+std::to_string(while_label_id), .instructions = {
                IRInstruction{ IROpcode::LABEL, irb.create_label(true, while_label_id) }
            } });
            irb.build_scope(stmt_while->scope, parent_block_index + 2);
            irb.m_current_block->instructions.push_back({ IROpcode::GOTO,
                irb.create_label(false, while_label_id) });

            if (parent_block_index <= irb.m_current_func->main_control_flow_index) irb.m_current_func->main_control_flow_index++;
            irb.m_current_block = &*irb.m_current_func->blocks.insert(irb.m_current_func->blocks.begin() + parent_block_index + 2,
                            { .name = "while_exit" + std::to_string(while_label_id), .instructions = {} });
        }
        void operator()(const NodeStmtPrint* stmt_print) const
        {
            if (irb.m_current_block == nullptr)
            {
                const IRBasicBlock print_block = { .name = "print", .instructions = {} };
                irb.m_current_func->blocks.push_back(print_block);
                irb.m_current_block = &irb.m_current_func->blocks.back();
            }
            const IROperand reg = irb.build_expr(stmt_print->expr);

            const IRInstruction print = { .opcode = IROpcode::PRINT, .src1 = reg };
            irb.m_current_block->instructions.push_back(print);
        }
        void operator()(const NodeStmtFunc* stmt_func) const
        {
            //prev state
            const auto current_func = irb.m_current_func->name;
            const auto current_block = irb.m_current_block->name;

            const auto func_name = std::string(stmt_func->ident.value.value());

            //setup
            irb.m_current_program->functions.push_back({ func_name, {
                { .name = func_name + "0" , .instructions = {
                    { IROpcode::LABEL, irb.create_label(true, func_name) }
                }}}});
            //changing to function writing
            irb.m_current_func = &irb.m_current_program->functions.back();
            irb.m_current_block = &irb.m_current_func->blocks.back();

            //save state for func scope
            const std::vector<VarInfo> saved_vars = irb.m_vars;
            const std::vector<size_t> saved_scopes = irb.m_scopes;
            irb.m_vars.clear();
            irb.m_scopes.clear();

            std::vector<VarInfo> params;
            //handle params!!
            if (stmt_func->params.has_value())
            {
                const auto& args = stmt_func->params.value();
                for (int i = 0; i < args.size(); i++)
                {
                    const auto param = VarInfo { irb.create_vreg(), args[i].type, std::string(args[i].ident.value.value()) };
                    params.push_back(param);
                    irb.m_vars.push_back(param);
                }
            }
            //store function
            irb.m_funcs.push_back({ params, func_name, {irb.create_vreg(), stmt_func->return_type } });
            //get IRInstructions
            irb.build_scope(stmt_func->scope, 0);

            //restore state
            irb.m_vars = saved_vars;
            irb.m_scopes = saved_scopes;

            //def exit
            irb.m_current_block->instructions.push_back({ IROpcode::GOTO, irb.create_label(false, func_name) });

            //switch back to previous state
            irb.m_current_func = &*std::ranges::find_if(irb.m_current_program->functions, [&](const IRFunction& func) {
                return func.name == current_func;
            });
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block;
            });
        }
        void operator()(const NodeStmtFuncCall* stmt_func_call) const
        {
            const auto it = std::ranges::find_if(std::as_const(irb.m_funcs), [&](const FuncInfo& func){
                            return func.name == stmt_func_call->ident.value.value();
                        });
            assert(it != irb.m_funcs.cend() && "Undeclared function identifier");

            //handle params
            if (stmt_func_call->exprs.has_value())
            {
                const auto& args = stmt_func_call->exprs.value();
                for (int i = 0; i < args.size(); i++)
                {
                    //because we need the registers so that we know where to put the generated expressions
                    //we need to declare a function before using it however i want to solve this in the future
                    //such that we would process functions definitions first somehow
                    irb.m_current_block->instructions.push_back({
                        IROpcode::COPY, it->params[i].reg, irb.build_expr(args[i])
                    });
                }
            }

            irb.m_current_block->instructions.push_back({ IROpcode::GOTO, irb.create_label(true, it->name) });

            if (parent_block_index <= irb.m_current_func->main_control_flow_index) irb.m_current_func->main_control_flow_index++;
            irb.m_current_block = &*irb.m_current_func->blocks.insert(irb.m_current_func->blocks.begin() + parent_block_index + 1,
                            { .name = "func_exit" + it->name, .instructions = {} });

            irb.m_current_block->instructions.push_back({ IROpcode::LABEL, irb.create_label(false, it->name) });
        }
        void operator()(const NodeStmtReturn* stmt_return) const
        {
            const auto it = std::ranges::find_if(std::as_const(irb.m_funcs), [&](const FuncInfo& func){
                            return func.name == irb.m_current_func->name;
                        });
            assert(it != irb.m_funcs.cend() && "Undeclared function identifier");

            if (stmt_return->expr.has_value())
            {
                const auto expr_reg = irb.build_expr(stmt_return->expr.value());
                irb.m_current_block->instructions.push_back({IROpcode::COPY, it->return_var.reg, expr_reg });
            }
            else
            {
                irb.m_current_block->instructions.push_back({IROpcode::COPY, it->return_var.reg, IROperand::make_lit(0) });
            }
            irb.m_current_block->instructions.push_back({ IROpcode::GOTO, irb.create_label(false, it->name) });
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

    StmtVisitor visitor { .irb = *this, .parent_block_index = parent_block_index };
    std::visit(visitor, stmt->stmt);
}

void IRBuilder::build_if_predicate(const NodeIfPredicate* pred, const size_t end_label_id, const int parent_block_index) {
    struct PredVisitor {
        IRBuilder& irb;
        const size_t end_label_id;
        int parent_block_index;
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
            irb.m_current_block = &*irb.m_current_func->blocks.insert(irb.m_current_func->blocks.begin() + parent_block_index + 1,
                { .name = "elseif"+std::to_string(else_if_in_label_id), .instructions = {
                IRInstruction{ IROpcode::LABEL, irb.create_label(true, else_if_in_label_id) }
            } });
            irb.build_scope(elseif_->scope, parent_block_index + 1);
            irb.m_current_block->instructions.push_back({ IROpcode::GOTO,
                irb.create_label(false, end_label_id) });

            //generate predicates if there are any
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block_name;
            });
            if (elseif_->ifpred.has_value())
            {
                irb.build_if_predicate(elseif_->ifpred.value(), else_if_in_label_id, parent_block_index);
            }
            irb.m_current_block = &*std::ranges::find_if(irb.m_current_func->blocks, [&](const IRBasicBlock& block) {
                return block.name == current_block_name;
            });
        }
        void operator()(const NodeIfPredElse* else_) const
        {
            irb.build_scope(else_->scope, parent_block_index);
        }
    };

    PredVisitor visitor{ .irb = *this, .end_label_id = end_label_id, .parent_block_index = parent_block_index };
    std::visit(visitor, pred->ifpred);
}

void IRBuilder::build_scope(const NodeScope* scope, const int parent_block_index) {
    begin_scope();
    for (const NodeStmt* stmt : scope->stmts)
    {
        build_statement(stmt, parent_block_index);
    }
    end_scope();
}
