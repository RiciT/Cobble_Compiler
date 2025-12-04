#include "optimizer.hpp"

#include <vector>
#include <algorithm>

IROptimizer::IROptimizer(IRProgram &prog)
    : m_prog(prog)
{
}

void IROptimizer::optimize()
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        changed |= dead_assignment_elimination();
        //and all other optimizations
    }
}

bool IROptimizer::dead_assignment_elimination()
{
    bool changed = false;
    //find all instances of vregs that are assigned but never used
    return changed;
}


