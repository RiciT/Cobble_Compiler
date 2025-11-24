#pragma once

#include <unordered_map>
#include <vector>

#include "ast/nodes.hpp"
#include "ir.hpp"

class IRBuilder {
public:
    explicit IRBuilder(NodeProgram prog);

    [[nodiscard]] IRProgram generate_ir();

private:
    void generate_stmt(const NodeStmt* stmt);

    void generate_if_predicate(const NodeIfPredicate *pred, const std::string &end_label);

    void generate_scope(const NodeScope* scope);

    IROperand generate_expr(const NodeExpr* expr);
    IROperand generate_binexpr(const NodeBinExpr* bin_expr);
    IROperand generate_atom(const NodeAtom* atom);

    //helpers
    IROperand create_vreg() const;
    std::string create_label();
    void emit(const IRInstruction &instr) const;

    struct VarInfo {
        IROperand reg; //the vreg holding this variable
        bool is_array;
        size_t array_size;
    };

    void push_scope();
    void pop_scope();
    VarInfo* find_var(const std::string& name);
    void add_var(const std::string& name, VarInfo var);

    //state
    NodeProgram m_prog;
    IRProgram m_result;

    //pointer to the function being built
    IRFunction* m_current_func = nullptr;
    //pointer to the block being built
    IRBasicBlock* m_current_block = nullptr;

    size_t m_label_counter = 0;

    //stack of symbol tables (name -> vreg)
    std::vector<std::unordered_map<std::string, VarInfo>> m_scopes;
};