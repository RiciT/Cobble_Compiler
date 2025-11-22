#pragma once

#include <variant>
#include <optional>
#include <vector>

#include "lexing/token.hpp"
#include "common/var_types.hpp"

#pragma region Forward declarations
struct NodeExpr;
struct NodeStmt;
struct NodeIfPredicate;
#pragma endregion

#pragma region Atoms
struct NodeAtomIntLit {
	Token int_lit;
};

struct NodeAtomBoolLit {
	Token bool_lit;
};

struct NodeAtomIdent {
	Token ident;
};

struct NodeAtomArrayAccess {
	Token ident;
	NodeExpr* index;
};

struct NodeAtomParen {
	NodeExpr* expr;
};

struct NodeAtom {
	std::variant<NodeAtomIntLit*, NodeAtomIdent*, NodeAtomParen*, NodeAtomBoolLit*, NodeAtomArrayAccess*> primary_expr;
};
#pragma endregion

#pragma region Binary Expressions
struct NodeBinExprAdd {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprMult {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprSub {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprDiv {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprEq {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprNotEq {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprGreater {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprLess {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprGreaterEq {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprLessEq {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExpr {
	std::variant<NodeBinExprAdd*, NodeBinExprMult*, NodeBinExprDiv*, NodeBinExprSub*,
		NodeBinExprEq*, NodeBinExprNotEq*, NodeBinExprGreaterEq*, NodeBinExprLessEq*,
		NodeBinExprLess*, NodeBinExprGreater*> bin_expr;
};
#pragma endregion

#pragma region Expressions
struct NodeFuncCallExpr {
	Token ident;
	std::optional<std::vector<NodeExpr*>> exprs;
};

struct NodeExpr {
	std::variant<NodeAtom*, NodeBinExpr*, NodeFuncCallExpr*> expr;
};
#pragma endregion

#pragma region Scope
struct NodeScope {
	std::vector<NodeStmt*> stmts;
};
#pragma endregion

#pragma region If Predicates
struct NodeIfPredElseIf {
	NodeExpr* expr{};
	NodeScope* scope{};
	std::optional<NodeIfPredicate*> ifpred;
};

struct NodeIfPredElse {
	NodeScope* scope;
};

struct NodeIfPredicate {
	std::variant<NodeIfPredElseIf*, NodeIfPredElse*> ifpred;
};
#pragma endregion

#pragma region Function Parameter
struct NodeFuncParam {
	VarType type;
	Token ident;
};
#pragma endregion

#pragma region Statements
struct NodeStmtPrint {
	NodeExpr* expr;
};

struct NodeStmtExit {
	NodeExpr* expr;
};

struct NodeStmtDef {
	VarType type;
	Token ident;
	std::optional<NodeExpr*> expr{};
	std::optional<NodeExpr*> array_size_expr{};
};

struct NodeStmtFunc {
	VarType return_type;
	Token ident;
	std::optional<std::vector<NodeFuncParam>> params;
	NodeScope* scope{};
};

struct NodeStmtFuncCall {
	Token ident;
	std::optional<std::vector<NodeExpr*>> exprs;
};

struct NodeStmtReturn {
	std::optional<NodeExpr*> expr;
};

struct NodeStmtIf {
	NodeExpr* expr{};
	NodeScope* scope{};
	std::optional<NodeIfPredicate*> ifpred;
};

struct NodeStmtAssign {
	Token ident;
	NodeExpr* expr{};
};

struct NodeStmtWhile {
	NodeExpr* expr;
	NodeScope* scope;
};

struct NodeStmtArrayAssign {
	Token ident;
	NodeExpr* index;
	NodeExpr* value;
};

struct NodeStmt {
	std::variant<NodeStmtExit*, NodeStmtPrint*, NodeStmtDef*, NodeScope*, NodeStmtIf*,
	NodeStmtAssign*, NodeStmtWhile*, NodeStmtFunc*, NodeStmtFuncCall*, NodeStmtReturn*,
	NodeStmtArrayAssign*> stmt;
};
#pragma endregion

struct NodeProgram {
	std::vector<NodeStmt*> stmts;
};
