#include "ir_builder.hpp"

IROperand IRBuilder::build_expr(const NodeExpr* expr) {
    struct ExprVisitor {
        IRBuilder& irb;
        void operator()(const NodeAtom* atom) const
        {
            // gen.generate_atom(atom);
        }
        void operator()(const NodeBinExpr* bin_expr) const
        {
            // gen.generate_binary_expression(bin_expr);
        }
        void operator()(const NodeFuncCallExpr* expr_func_call) const
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
    std::visit(visitor, expr->expr);
}

IROperand IRBuilder::build_binexpr(const NodeBinExpr* bin_expr) {
struct BinExprVisitor {
        IRBuilder& irb;
        void operator()(const NodeBinExprAdd* add) const {
            // gen.generate_expression(add->rhs);
            // gen.generate_expression(add->lhs);
            // gen.pop("rax");
            // gen.pop("rbx");
            // gen.m_emitter.emit("add", "rax", "rbx");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprSub* sub) const {
            // gen.generate_expression(sub->rhs);
            // gen.generate_expression(sub->lhs);
            // gen.pop("rax");
            // gen.pop("rbx");
            // gen.m_emitter.emit("sub", "rax", "rbx");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprMult* mult) const {
            // gen.generate_expression(mult->rhs);
            // gen.generate_expression(mult->lhs);
            // gen.pop("rax");
            // gen.pop("rbx");
            // gen.m_emitter.emit("mul", "rbx");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprDiv* div) const {
            // gen.generate_expression(div->rhs);
            // gen.generate_expression(div->lhs);
            // gen.pop("rax");
            // gen.pop("rbx");
            // gen.m_emitter.emit("xor", "rdx", "rdx");
            // gen.m_emitter.emit("div", "rbx");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprEq* eq) const {
            // gen.generate_expression(eq->lhs);
            // gen.generate_expression(eq->rhs);
            // gen.pop("rbx");
            // gen.pop("rax");
            // gen.m_emitter.emit("cmp", "rax", "rbx");
            // gen.m_emitter.emit("sete", "al"); //set al to 1 if equal
            // gen.m_emitter.emit("movzx", "rax", "al"); //zero-extend to full register
            // gen.push("rax");
        }
        void operator()(const NodeBinExprNotEq* neq) const {
            // gen.generate_expression(neq->lhs);
            // gen.generate_expression(neq->rhs);
            // gen.pop("rbx");
            // gen.pop("rax");
            // gen.m_emitter.emit("cmp", "rax", "rbx");
            // gen.m_emitter.emit("setne", "al");
            // gen.m_emitter.emit("movzx", "rax", "al");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprGreater* gt) const {
            // gen.generate_expression(gt->lhs);
            // gen.generate_expression(gt->rhs);
            // gen.pop("rbx");
            // gen.pop("rax");
            // gen.m_emitter.emit("cmp", "rax", "rbx");
            // gen.m_emitter.emit("setg", "al");
            // gen.m_emitter.emit("movzx", "rax", "al");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprLess* lt) const {
            // gen.generate_expression(lt->lhs);
            // gen.generate_expression(lt->rhs);
            // gen.pop("rbx");
            // gen.pop("rax");
            // gen.m_emitter.emit("cmp", "rax", "rbx");
            // gen.m_emitter.emit("setl", "al");
            // gen.m_emitter.emit("movzx", "rax", "al");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprGreaterEq* gte) const {
            // gen.generate_expression(gte->lhs);
            // gen.generate_expression(gte->rhs);
            // gen.pop("rbx");
            // gen.pop("rax");
            // gen.m_emitter.emit("cmp", "rax", "rbx");
            // gen.m_emitter.emit("setge", "al");
            // gen.m_emitter.emit("movzx", "rax", "al");
            // gen.push("rax");
        }
        void operator()(const NodeBinExprLessEq* lte) const {
            // gen.generate_expression(lte->lhs);
            // gen.generate_expression(lte->rhs);
            // gen.pop("rbx");
            // gen.pop("rax");
            // gen.m_emitter.emit("cmp", "rax", "rbx");
            // gen.m_emitter.emit("setle", "al");
            // gen.m_emitter.emit("movzx", "rax", "al");
            // gen.push("rax");
        }
    };
    BinExprVisitor visitor { .irb = *this };
    std::visit(visitor, bin_expr->bin_expr);
}

IROperand IRBuilder::build_atom(const NodeAtom* atom) {
struct AtomVisitor {
        IRBuilder& irb;
        void operator()(const NodeAtomIdent* atom_ident) const{
            // const auto it = std::ranges::find_if(std::as_const(gen.m_vars), [&](const Variable& var){
            //     return var.name == atom_ident->ident.value.value(); });
            //
            // assert(it != gen.m_vars.cend() && "Undeclared identifier");
            //
            // if (it->is_param)
            // {
            //     //parameters: positive offset from rbp
            //     std::stringstream offset;
            //     offset << "QWORD [rbp + " << it->stack_loc << "]";
            //     gen.push(offset.str());
            // }
            // else
            // {
            //     //local variables: calculated from rsp
            //     std::stringstream offset;
            //     offset << "QWORD [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "]";
            //     gen.push(offset.str());
            // }
        }
        void operator()(const NodeAtomIntLit* atom_int_lit) const {

            // gen.m_emitter.emit("mov", "rax", atom_int_lit->int_lit.value.value());
            // gen.push("rax");
        }
        void operator()(const NodeAtomBoolLit* atom_bool_lit) const
        {
            //true = 1, false = 0
            // if (atom_bool_lit->bool_lit.type == TokenType::true_)
            // {
            //     gen.m_emitter.emit("mov", "rax", "1");
            // }
            // if (atom_bool_lit->bool_lit.type == TokenType::false_)
            // {
            //     gen.m_emitter.emit("mov", "rax", "0");
            // }
            // gen.push("rax");
        }
        void operator()(const NodeAtomParen* atom_paren) const {
            // gen.generate_expression(atom_paren->expr);
        }
        void operator()(const NodeAtomArrayAccess* atom_array_access) const
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
    std::visit(visitor, atom->primary_expr);
}
