#include <algorithm>
#include <ranges>
#include <variant>
#include <functional>

#include "type_checker.hpp"
#include "parsing/operator_precedence.hpp"

//const helpers (in anon namespace so they are stateless)
namespace {

    //forward declaration of the recursive function
    std::optional<int64_t> evaluate_const_expr(const NodeExpr* expr);

    //atom visitor
    struct ConstAtomVisitor {
        std::optional<int64_t> operator()(const NodeAtomIntLit* int_lit) const {
            return std::stoll(std::string(int_lit->int_lit.value.value()));
        }
        std::optional<int64_t> operator()(const NodeAtomBoolLit* bool_lit) const {
            return bool_lit->bool_lit.type == TokenType::true_ ? 1 : 0;
        }
        std::optional<int64_t> operator()(const NodeAtomParen* paren) const {
            return evaluate_const_expr(paren->expr);
        }
        //variables and array access cannot be compile-time constants in this context
        std::optional<int64_t> operator()(const NodeAtomIdent*) const { return {}; }
        std::optional<int64_t> operator()(const NodeAtomArrayAccess*) const { return {}; }
    };

    //binary expression visitor
    struct ConstBinVisitor {
        std::optional<int64_t> operator()(const NodeBinExprAdd* add) const {
            const auto lhs = evaluate_const_expr(add->lhs);
            if (const auto rhs = evaluate_const_expr(add->rhs); lhs && rhs) return *lhs + *rhs;
            return {};
        }
        std::optional<int64_t> operator()(const NodeBinExprSub* sub) const {
            const auto lhs = evaluate_const_expr(sub->lhs);
            if (const auto rhs = evaluate_const_expr(sub->rhs); lhs && rhs) return *lhs - *rhs;
            return {};
        }
        std::optional<int64_t> operator()(const NodeBinExprMult* mult) const {
            const auto lhs = evaluate_const_expr(mult->lhs);
            if (const auto rhs = evaluate_const_expr(mult->rhs); lhs && rhs) return *lhs * *rhs;
            return {};
        }
        std::optional<int64_t> operator()(const NodeBinExprDiv* div) const {
            const auto lhs = evaluate_const_expr(div->lhs);
            if (const auto rhs = evaluate_const_expr(div->rhs); lhs && rhs && *rhs != 0) return *lhs / *rhs;
            return {};
        }
        //template catch-all
        template <typename T>
        std::optional<int64_t> operator()(const T*) const { return {}; }
    };

    //expression visitor
    struct ConstExprVisitor {
        std::optional<int64_t> operator()(const NodeAtom* atom) const {
            return std::visit(ConstAtomVisitor{}, atom->primary_expr);
        }
        std::optional<int64_t> operator()(const NodeBinExpr* bin) const {
            return std::visit(ConstBinVisitor{}, bin->bin_expr);
        }
        std::optional<int64_t> operator()(const NodeFuncCallExpr*) const { return {}; }
    };

    //implement the recursive function using the visitors
    std::optional<int64_t> evaluate_const_expr(const NodeExpr* expr) {
        return std::visit(ConstExprVisitor{}, expr->expr);
    }
}

//actual implementation
TypeChecker::TypeChecker(NodeProgram prog, ErrorHandler &error_handler)
    : m_prog(std::move(prog)),
      m_error_handler(error_handler)
{
}

void TypeChecker::begin_scope()
{
    m_scopes.push_back(m_vars.size());
}

void TypeChecker::end_scope()
{
    const size_t restore_point = m_scopes.back();
    m_scopes.pop_back();
    if (m_vars.size() > restore_point) {
        m_vars.erase(m_vars.begin() + restore_point, m_vars.end());
    }
}

//recursively analyse expressions
VarType TypeChecker::analyse_expr(NodeExpr *expr)
{
    struct ExprVisitor {
        TypeChecker& tc;

        VarType operator()(const NodeAtom* atom) const {
            struct AtomTypeVisitor {
                TypeChecker& tc;
                const NodeAtom* parent;

                VarType operator()(const NodeAtomIntLit*) const {
                    return VarType::make_int_lit();
                }
                VarType operator()(const NodeAtomBoolLit*) const {
                    return VarType::make_bool_lit();
                }
                VarType operator()(const NodeAtomIdent* ident) const {
                    const auto it = std::ranges::find_if(tc.m_vars, [&](const Variable& v) {
                        return v.name == ident->ident.value.value();
                    });

                    if (it == tc.m_vars.end()) {
                        tc.m_error_handler.report("Undeclared identifier: " + std::string(ident->ident.value.value()), ident->ident);
                        return VarType::make_int_lit(); //dummy return
                    }
                    return it->type;
                }
                VarType operator()(const NodeAtomParen* paren) const {
                    return tc.analyse_expr(paren->expr);
                }
                VarType operator()(const NodeAtomArrayAccess* acc) const {
                    const auto it = std::ranges::find_if(tc.m_vars, [&](const Variable& v) {
                        return v.name == acc->ident.value.value();
                    });
                    if (it == tc.m_vars.end()) {
                        tc.m_error_handler.report("Undeclared identifier: " + std::string(acc->ident.value.value()), acc->ident);
                        return VarType::make_int_lit();
                    }
                    if (!it->type.is_array) {
                        tc.m_error_handler.report("Cannot index non-array variable", acc->ident);
                    }
                    //check index type
                    if (const VarType idx_type = tc.analyse_expr(acc->index); idx_type.base != BaseType::int_) {
                        tc.m_error_handler.report("Array index must be an integer", acc->ident);
                    }
                    return it->type.element_type();
                }
            };
            return std::visit(AtomTypeVisitor{tc, atom}, atom->primary_expr);
        }

        VarType operator()(const NodeBinExpr* bin) const {
            //helper to check operands
            auto check_arithmetic = [&](NodeExpr* lhs, NodeExpr* rhs) {
                const VarType t1 = tc.analyse_expr(lhs);
                if (const VarType t2 = tc.analyse_expr(rhs); t1.base != BaseType::int_ || t2.base != BaseType::int_) {
                    tc.m_error_handler.report("Arithmetic operations require integer operands", 0);
                }
                return VarType::make_int_lit();
            };

            auto check_comparison = [&](NodeExpr* lhs, NodeExpr* rhs) {
                const VarType t1 = tc.analyse_expr(lhs);
                if (const VarType t2 = tc.analyse_expr(rhs); t1 != t2) {
                    tc.m_error_handler.report("Comparison requires matching types", 0);
                }
                return VarType::make_bool_lit();
            };

            struct BinTypeVisitor {
                std::function<VarType(NodeExpr*, NodeExpr*)> arith;
                std::function<VarType(NodeExpr*, NodeExpr*)> comp;

                VarType operator()(const NodeBinExprAdd* e) const { return arith(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprSub* e) const { return arith(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprMult* e) const { return arith(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprDiv* e) const { return arith(e->lhs, e->rhs); }

                VarType operator()(const NodeBinExprEq* e) const { return comp(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprNotEq* e) const { return comp(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprGreater* e) const { return comp(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprLess* e) const { return comp(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprGreaterEq* e) const { return comp(e->lhs, e->rhs); }
                VarType operator()(const NodeBinExprLessEq* e) const { return comp(e->lhs, e->rhs); }
            };

            return std::visit(BinTypeVisitor{check_arithmetic, check_comparison}, bin->bin_expr);
        }

        VarType operator()(const NodeFuncCallExpr* call) const {
             const auto func_it = std::ranges::find_if(tc.m_prog.stmts, [&](const NodeStmt* stmt){
                if (std::holds_alternative<NodeStmtFunc*>(stmt->stmt)) {
                    return std::get<NodeStmtFunc*>(stmt->stmt)->ident.value.value() == call->ident.value.value();
                }
                return false;
            });

            if (func_it == tc.m_prog.stmts.end()) {
                tc.m_error_handler.report("Call to undefined function: " + std::string(call->ident.value.value()), call->ident);
                return VarType::make_int_lit();
            }

            const auto func_def = std::get<NodeStmtFunc*>((*func_it)->stmt);
            const size_t call_args = call->exprs.has_value() ? call->exprs.value().size() : 0;
             if (const size_t def_args = func_def->params.has_value() ? func_def->params.value().size() : 0; call_args != def_args) {
                tc.m_error_handler.report("Function argument count mismatch", call->ident);
            }

            return func_def->return_type;
        }
    };

    return std::visit(ExprVisitor{*this}, expr->expr);
}

void TypeChecker::analyse_stmt(NodeStmt *stmt)
{
    struct StmtVisitor {
        TypeChecker& tc;

        void operator()(const NodeStmtExit* stmt_exit) const {
            if (const VarType type = tc.analyse_expr(stmt_exit->expr); type.base != BaseType::int_) {
                tc.m_error_handler.report("Exit code must be an integer", 0);
            }
        }

        void operator()(const NodeStmtPrint* stmt_print) const {
            if (const VarType type = tc.analyse_expr(stmt_print->expr); type.base != BaseType::int_) {
                tc.m_error_handler.report("Print statement currently only supports integers", 0);
            }
        }

        void operator()(const NodeStmtDef* stmt_def) const {
            const auto it = std::ranges::find_if(tc.m_vars, [&](const Variable& v){
                return v.name == stmt_def->ident.value.value();
            });

            if (it != tc.m_vars.end()) {
                tc.m_error_handler.report("Variable redefinition: " + std::string(stmt_def->ident.value.value()), stmt_def->ident);
                return;
            }

            if (stmt_def->type.is_array) {
                if (!stmt_def->array_size_expr.has_value()) {
                     tc.m_error_handler.report("Array requires size", stmt_def->ident);
                     return;
                }
                const auto size = evaluate_const_expr(stmt_def->array_size_expr.value());
                if (!size.has_value() || size.value() <= 0) {
                     tc.m_error_handler.report("Array size must be a positive constant", stmt_def->ident);
                }

                VarType final_type = stmt_def->type;
                if (size.has_value()) final_type.array_size = static_cast<size_t>(size.value());

                tc.m_vars.push_back({ .name = std::string(stmt_def->ident.value.value()), .type = final_type });
            } else {
                if (stmt_def->expr.has_value()) {
                    if (const VarType expr_type = tc.analyse_expr(stmt_def->expr.value()); expr_type != stmt_def->type) {
                        tc.m_error_handler.report("Type mismatch in variable definition", stmt_def->ident);
                    }
                }
                tc.m_vars.push_back({ .name = std::string(stmt_def->ident.value.value()), .type = stmt_def->type });
            }
        }

        void operator()(const NodeStmtAssign* stmt_assign) const {
            const auto it = std::ranges::find_if(tc.m_vars, [&](const Variable& v){
                return v.name == stmt_assign->ident.value.value();
            });

            if (it == tc.m_vars.end()) {
                tc.m_error_handler.report("Undeclared identifier in assignment: " + std::string(stmt_assign->ident.value.value()), stmt_assign->ident);
                return;
            }

            if (const VarType expr_type = tc.analyse_expr(stmt_assign->expr); expr_type != it->type) {
                tc.m_error_handler.report("Type mismatch in assignment", stmt_assign->ident);
            }
        }

        void operator()(const NodeStmtArrayAssign* stmt_arr) const {
            const auto it = std::ranges::find_if(tc.m_vars, [&](const Variable& v){
                return v.name == stmt_arr->ident.value.value();
            });
            if (it == tc.m_vars.end()) {
                tc.m_error_handler.report("Undeclared identifier: " + std::string(stmt_arr->ident.value.value()), stmt_arr->ident);
                return;
            }
            if (!it->type.is_array) {
                tc.m_error_handler.report("Variable is not an array", stmt_arr->ident);
                return;
            }

            if (const VarType index_type = tc.analyse_expr(stmt_arr->index); index_type.base != BaseType::int_) {
                tc.m_error_handler.report("Array index must be integer", stmt_arr->ident);
            }

             if (const VarType val_type = tc.analyse_expr(stmt_arr->value); val_type != it->type.element_type()) {
                 tc.m_error_handler.report("Type mismatch in array assignment", stmt_arr->ident);
            }
        }

        void operator()(const NodeScope* scope) const {
            tc.analyse_scope(const_cast<NodeScope*>(scope));
        }

        void operator()(const NodeStmtIf* stmt_if) const {
            tc.analyse_expr(stmt_if->expr);
            tc.analyse_scope(stmt_if->scope);
            if (stmt_if->ifpred.has_value()) {
                 struct PredVisitor {
                    TypeChecker& tc;
                    void operator()(const NodeIfPredElseIf* elif) const {
                        tc.analyse_expr(elif->expr);
                        tc.analyse_scope(elif->scope);
                        if (elif->ifpred.has_value()) std::visit(*this, elif->ifpred.value()->ifpred);
                    }
                    void operator()(const NodeIfPredElse* els) const {
                        tc.analyse_scope(els->scope);
                    }
                 };
                 std::visit(PredVisitor{tc}, stmt_if->ifpred.value()->ifpred);
            }
        }

        void operator()(const NodeStmtWhile* stmt_while) const {
            tc.analyse_expr(stmt_while->expr);
            tc.analyse_scope(stmt_while->scope);
        }

        void operator()(const NodeStmtFunc* stmt_func) const {
            const std::vector<Variable> saved_vars = tc.m_vars;
            const std::vector<size_t> saved_scopes = tc.m_scopes;

            tc.m_vars.clear();
            tc.m_scopes.clear();

            if (stmt_func->params.has_value()) {
                for(const auto&[type, ident] : stmt_func->params.value()) {
                    tc.m_vars.push_back({ .name = std::string(ident.value.value()), .type = type });
                }
            }

            tc.analyse_scope(stmt_func->scope);

            tc.m_vars = saved_vars;
            tc.m_scopes = saved_scopes;
        }

        void operator()(const NodeStmtFuncCall* call) const {
             const auto func_it = std::ranges::find_if(tc.m_prog.stmts, [&](const NodeStmt* stmt){
                if (std::holds_alternative<NodeStmtFunc*>(stmt->stmt)) {
                    return std::get<NodeStmtFunc*>(stmt->stmt)->ident.value.value() == call->ident.value.value();
                }
                return false;
            });

            if (func_it == tc.m_prog.stmts.end()) {
                tc.m_error_handler.report("Call to undefined function: " + std::string(call->ident.value.value()), call->ident);
                return;
            }

            const auto func_def = std::get<NodeStmtFunc*>((*func_it)->stmt);
            const size_t call_args = call->exprs.has_value() ? call->exprs.value().size() : 0;
            if (const size_t def_args = func_def->params.has_value() ? func_def->params.value().size() : 0; call_args != def_args) {
                tc.m_error_handler.report("Argument count mismatch", call->ident);
            }
        }

        void operator()(const NodeStmtReturn* ret) const {
            if (ret->expr.has_value()) {
                tc.analyse_expr(ret->expr.value());
            }
        }
    };

    std::visit(StmtVisitor{*this}, stmt->stmt);
}

void TypeChecker::analyse_scope(const NodeScope* scope)
{
    begin_scope();
    for (NodeStmt* stmt : scope->stmts) {
        analyse_stmt(stmt);
    }
    end_scope();
}

void TypeChecker::analyse_program()
{
    for (const NodeStmt* stmt : m_prog.stmts)
    {
        analyse_stmt(const_cast<NodeStmt*>(stmt));
    }
}
