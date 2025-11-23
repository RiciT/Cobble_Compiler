#pragma once

#include <vector>
#include <string>

#include  "ast/nodes.hpp"
#include  "diagnostics/error_handler.hpp"

class TypeChecker {
public:
    explicit TypeChecker(NodeProgram prog, ErrorHandler& error_handler);

    void analyse_program();

private:
    void analyse_scope(NodeScope* scope);
    void analyse_stmt(NodeStmt* stmt);
    void analyse_expr(NodeExpr* expr);

    //Logic extracted from Generator
    struct Variable {
        std::string name;
        VarType type;
    };

    const NodeProgram m_prog;
    ErrorHandler& m_error_handler;

    std::vector<Variable> m_vars;
    std::vector<size_t> m_scopes;

    void begin_scope();
    void end_scope();
};