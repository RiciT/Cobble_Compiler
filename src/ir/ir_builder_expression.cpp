#include <cassert>

#include "ir_builder.hpp"

IROperand IRBuilder::build_expr(const NodeExpr* expr) {
    struct ExprVisitor {
        IRBuilder& irb;
        IROperand operator()(const NodeAtom* atom) const
        {
            return irb.build_atom(atom);
        }
        IROperand operator()(const NodeBinExpr* bin_expr) const
        {
            return irb.build_binexpr(bin_expr);
        }
        IROperand operator()(const NodeFuncCallExpr* expr_func_call) const
        {
            //push arguments right-to-left
            // if (expr_func_call->exprs.has_value())
            // {
            //     const auto& args = expr_func_call->exprs.value();
            //     for (int i = args.size() - 1; i >= 0; i--)
            //     {
            //         gen.generate_expression(args[i]);
            //         //expression result is already pushed
            //     }
            // }
            //
            // //push parameters onto stack so it can be popped in order
            // const std::string func_label = "func_" + std::string(expr_func_call->ident.value.value());
            // gen.m_emitter.emit("call", func_label);
            //
            // //clean up arguments from stack
            // if (expr_func_call->exprs.has_value())
            // {
            //     if (const size_t args_size = expr_func_call->exprs.value().size(); args_size > 0)
            //     {
            //         gen.m_emitter.emit("add", "rsp", args_size * 8);
            //         gen.m_stack_size -= args_size;
            //     }
            // }
            // gen.push("rax");
        }
    };

    ExprVisitor visitor{ .irb = *this };
    return std::visit(visitor, expr->expr);
}

IROperand IRBuilder::build_binexpr(const NodeBinExpr* bin_expr) {
    struct BinExprVisitor {
        IRBuilder& irb;
        IROperand operator()(const NodeBinExprAdd* add) const
        {
            const auto reg1 = irb.build_expr(add->lhs);
            const auto reg2 = irb.build_expr(add->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::ADD, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprSub* sub) const
        {
            const auto reg1 = irb.build_expr(sub->lhs);
            const auto reg2 = irb.build_expr(sub->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::SUB, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprMult* mult) const
        {
            const auto reg1 = irb.build_expr(mult->lhs);
            const auto reg2 = irb.build_expr(mult->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::MUL, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprDiv* div) const
        {
            const auto reg1 = irb.build_expr(div->lhs);
            const auto reg2 = irb.build_expr(div->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::DIV, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprEq* eq) const
        {
            const auto reg1 = irb.build_expr(eq->lhs);
            const auto reg2 = irb.build_expr(eq->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::EQ, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprNotEq* neq) const
        {
            const auto reg1 = irb.build_expr(neq->lhs);
            const auto reg2 = irb.build_expr(neq->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::NEQ, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprGreater* gt) const
        {
            const auto reg1 = irb.build_expr(gt->lhs);
            const auto reg2 = irb.build_expr(gt->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::GT, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprLess* lt) const
        {
            const auto reg1 = irb.build_expr(lt->lhs);
            const auto reg2 = irb.build_expr(lt->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::LT, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprGreaterEq* gte) const
        {
            const auto reg1 = irb.build_expr(gte->lhs);
            const auto reg2 = irb.build_expr(gte->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::GTE, reg, reg1, reg2 });
            return reg;
        }
        IROperand operator()(const NodeBinExprLessEq* lte) const
        {
            const auto reg1 = irb.build_expr(lte->lhs);
            const auto reg2 = irb.build_expr(lte->rhs);
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ IROpcode::LTE, reg, reg1, reg2 });
            return reg;
        }
    };
    BinExprVisitor visitor { .irb = *this };
    return std::visit(visitor, bin_expr->bin_expr);
}

IROperand IRBuilder::build_atom(const NodeAtom* atom) {
    struct AtomVisitor {
        IRBuilder& irb;
        IROperand operator()(const NodeAtomIdent* atom_ident) const{
            const auto it = std::ranges::find_if(std::as_const(irb.m_vars), [&](const VarInfo& var){
                return var.name == atom_ident->ident.value.value();
            });
            assert(it != irb.m_vars.cend() && "Undeclared identifier");

            return it->reg;
        }
        IROperand operator()(const NodeAtomIntLit* atom_int_lit) const {
            //now we are only handling it separately not inside an expr so this works fine for now
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ .opcode = IROpcode::COPY, .dest = reg,
                .src1 = IROperand::make_lit(atom_int_lit->int_lit.value.value()) });
            return reg;
        }
        IROperand operator()(const NodeAtomBoolLit* atom_bool_lit) const
        {
            //true = 1, false = 0
            const auto reg = irb.create_vreg();
            irb.m_current_block->instructions.push_back({ .opcode = IROpcode::COPY, .dest = reg,
                .src1 = IROperand::make_lit(atom_bool_lit->bool_lit.type == TokenType::true_ ? 1 :
                    atom_bool_lit->bool_lit.type == TokenType::false_ ? 0 : -1) });
            return reg;
        }
        IROperand operator()(const NodeAtomParen* atom_paren) const {
            return irb.build_expr(atom_paren->expr);
        }
        IROperand operator()(const NodeAtomArrayAccess* atom_array_access) const
        {
            // const auto it = std::ranges::find_if(gen.m_vars, [&](const Variable& var) {
            //     return var.name == atom_array_access->ident.value.value();
            // });
            //
            // assert(it != gen.m_vars.end() && "Undeclared identifier in array access");
            // assert(it->type.is_array && "Cannot index non-array");
            //
            // gen.generate_expression(atom_array_access->index);
            // gen.pop("rax");
            //
            // //calc offset
            // gen.m_emitter.emit("mov", "rbx", "8");
            // gen.m_emitter.emit("mul", "rbx");
            //
            // //address
            // const size_t base_offset = (gen.m_stack_size - it->stack_loc - it->type.array_size) * 8;
            //
            // gen.m_emitter.emit("mov", "rbx", "rsp");
            // gen.m_emitter.emit("add", "rbx", base_offset);
            // gen.m_emitter.emit("add", "rbx", "rax");
            //
            // gen.m_emitter.emit("mov", "rax", "[rbx]");
            // gen.push("rax");
        }
    };
    AtomVisitor visitor({ .irb = *this });
    return std::visit(visitor, atom->primary_expr);
}