#pragma once

#include <unordered_map>
#include <vector>

#include "ast/nodes.hpp"
#include "ir.hpp"
#include "common/arena_allocator.hpp"

class IRBuilder {
public:
    explicit IRBuilder(NodeProgram prog);

    [[nodiscard]] IRProgram build_ir();

private:
    void build_statement(const NodeStmt* stmt);
    void build_if_predicate(const NodeIfPredicate *pred, const std::string &end_label);
    void build_scope(const NodeScope* scope);
    IROperand build_expr(const NodeExpr* expr);
    IROperand build_binexpr(const NodeBinExpr* bin_expr);
    IROperand build_atom(const NodeAtom* atom);

    //helpers
    IROperand create_vreg() const;
    std::string create_label();
    void emit(const IRInstruction &instr) const;

    struct VarInfo {
        IROperand reg; //the vreg holding this variable
        VarType type;
        std::string name;
        bool is_array;
        size_t array_size;
    };

    void begin_scope();
    void end_scope();
    VarInfo* find_var(const std::string& name);
    void add_var(const std::string& name, VarInfo var);

    //state
    NodeProgram m_prog;

    //pointer to the function being built
    IRFunction* m_current_func = nullptr;
    //pointer to the block being built
    IRBasicBlock* m_current_block = nullptr;

    size_t m_label_counter = 0;
    //stack of symbol tables (name -> vreg)
    std::vector<std::unordered_map<std::string, VarInfo>> m_scopes;
};
