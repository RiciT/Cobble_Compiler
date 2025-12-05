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
    bool algebraic_strength_reduction();
    bool common_subexpression_elimination();
    bool dead_assignment_elimination();
    bool constant_propagation_block();

    //CFG (control flow graph) optimization
    bool unreachable_code_elimination();
    bool dead_code_elimination();
    bool coalescing(); //block merging - redunant jump elimination
    bool empty_block_removal();

    //dataflow optimization - global opt
    bool copy_propagation();
    bool constant_propagation_global();

    IRProgram& m_prog;
};