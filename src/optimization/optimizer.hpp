#pragma once

#include "ir/ir.hpp"

class IROptimizer {
public:
    explicit IROptimizer(IRProgram& prog);

    void optimize();

private:
    //optimization passes
    //basic block optimizers
    bool constant_folding();
    bool algebraic_reduction();
    bool reassignment_elimination();
    bool common_subexpression_elimination();
    bool dead_assignment_elimination();
    bool constant_propagation_block();

    //CFG (control flow graph) optimization
    bool unreachable_code_elimination();
    bool dead_code_elimination();
    bool coalescing(); //block merging - redunant jump elimination
    bool coalescing_single_jump_labels(long index_f, IRFunction& func);
    bool empty_block_removal();

    //dataflow optimization - global opt
    bool copy_propagation();

    IRProgram& m_prog;
};