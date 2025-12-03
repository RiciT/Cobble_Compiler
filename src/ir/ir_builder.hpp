#pragma once

#include <unordered_map>
#include <vector>

#include "ast/nodes.hpp"
#include "ir.hpp"

class IRBuilder {
public:
    explicit IRBuilder(NodeProgram prog);

    [[nodiscard]] IRProgram build_ir();

private:
    void build_statement(const NodeStmt* stmt);
    void build_if_predicate(const NodeIfPredicate *pred, size_t end_label_id);
    void build_scope(const NodeScope* scope);
    IROperand build_expr(const NodeExpr* expr);
    IROperand build_binexpr(const NodeBinExpr* bin_expr);
    IROperand build_atom(const NodeAtom* atom);

    //helpers
    IROperand create_vreg() const;

    IROperand create_label(bool isEnter, std::string name);
    IROperand create_label(bool isEnter, size_t id);
    IROperand create_label(bool isEnter);

    struct VarInfo {
        IROperand reg; //the vreg holding this variable
        VarType type;
        std::string name;
        bool is_array;
        size_t array_size;
    };

    void begin_scope();
    void end_scope();
    void flush_current_block();

    //state
    NodeProgram m_prog;

    //pointer to current program might not be needed but added in case i do
    IRProgram* m_current_program = nullptr;
    //pointer to the function being built
    IRFunction* m_current_func = nullptr;
    //pointer to the block being built
    IRBasicBlock* m_current_block = nullptr;
    //stack of symbol tables (name -> vreg)
    std::vector<std::unordered_map<std::string, VarInfo>> m_scopes {};
    std::vector<VarInfo> m_vars {};

    size_t m_label_id = 0;
};
