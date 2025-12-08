#pragma once

#include <vector>

#include "ast/nodes.hpp"
#include "ir.hpp"

class IRBuilder {
public:
    explicit IRBuilder(NodeProgram prog);

    [[nodiscard]] IRProgram build_ir();

private:
    void build_statement(const NodeStmt* stmt, int parent_block_index);
    void build_if_predicate(const NodeIfPredicate *pred, size_t end_label_id, int parent_block_index);
    void build_scope(const NodeScope* scope, int parent_block_index);
    IROperand build_expr(const NodeExpr* expr);
    IROperand build_binexpr(const NodeBinExpr* bin_expr);
    IROperand build_atom(const NodeAtom* atom);

    //helpers
    IROperand create_vreg() const;

    static IROperand create_label(bool isEnter, const std::string &name);
    static IROperand create_label(bool isEnter, size_t id);
    IROperand create_label(bool isEnter) const;

    struct VarInfo {
        IROperand reg; //the vreg holding this variable
        VarType type;
        std::string name;
    };

    struct FuncInfo {
        std::vector<VarInfo> params;
        std::string name;
        VarInfo return_var;
    };

    void begin_scope();
    void end_scope();

    //state
    NodeProgram m_prog;
    mutable size_t m_vreg_count = 0;
    mutable size_t m_label_id = 0;

    //pointer to current program might not be needed but added in case i do
    IRProgram* m_current_program = nullptr;
    //pointer to the function being built
    IRFunction* m_current_func = nullptr;
    //pointer to the block being built
    IRBasicBlock* m_current_block = nullptr;
    //store scopes
    std::vector<size_t> m_scopes {};
    std::vector<VarInfo> m_vars {};
    std::vector<FuncInfo> m_funcs {};
};
