#pragma once

#include "ast/nodes.hpp"
#include "asm_emitter.hpp"

class Generator {
public:
    explicit Generator(NodeProgram prog);

    [[nodiscard]] std::string generate_program();

private:
    void generate_atom(const NodeAtom* atom);
    void generate_binary_expression(const NodeBinExpr* bin_expr);
    void generate_expression(const NodeExpr* expr);
    void generate_scope(const NodeScope* scope);
    void generate_if_predicate(const NodeIfPredicate* pred, const std::string& end_label);
    void generate_statement(const NodeStmt* stmt);

    //temporary helpers for evaluating const expression
    std::optional<int64_t> evaluate_const_expr(const NodeExpr* expr);
    std::optional<int64_t> evaluate_const_atom(const NodeAtom* atom);
    std::optional<int64_t> evaluate_const_binexpr(const NodeBinExpr* bin_expr);

    //helpers
    void push(const std::string& reg);
    void pop(const std::string& reg);
    void begin_scope();
    void end_scope();
    std::string create_label();

    struct Variable 
    {
        VarType type;
        std::string name;
        size_t stack_loc;
        bool is_param = false;
    };

    const NodeProgram m_prog;

   AsmEmitter m_emitter;

    size_t m_stack_size = 0;
    std::vector<Variable> m_vars {};
    std::vector<size_t> m_scopes {};
    int m_label_count = 0;
};