#include <algorithm>

#include "type_checker.hpp"

TypeChecker::TypeChecker(NodeProgram prog, ErrorHandler &error_handler)
    : m_prog(std::move(prog)),
      m_error_handler(error_handler)
{
}

void TypeChecker::analyse_program()
{
    return;
}

void TypeChecker::analyse_stmt(NodeStmt *stmt)
{
    return;
}

void TypeChecker::analyse_expr(NodeExpr *expr)
{
    return;
}

void TypeChecker::analyse_scope(NodeScope *scope)
{
    return;
}

void TypeChecker::begin_scope()
{
    return;
}

void TypeChecker::end_scope()
{
    return;
}