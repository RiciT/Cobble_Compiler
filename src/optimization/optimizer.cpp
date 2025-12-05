#include "optimizer.hpp"

#include <vector>
#include <algorithm>
#include <ranges>

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

// ReSharper disable once CppMemberFunctionMayBeConst
//it is logiaclly not const since it modifies the referenced member m_prog
bool IROptimizer::dead_assignment_elimination()
{
    //this only removes trivial dead assignments so for example:
    /* if there is something like this:
    t0 = t1
    and then no t0 used anywhere as a src then it gets removed however,
    if there is something like this:
    t0 = t1
    t2 = t0 + 5
    t0 = t3
    and then t0 is not used anywhere as a src again then it will not be removed
    by this even though the last assignment is unnecessary (the first as well in
    this example but that will be handled by another optimizing function) */
    bool changed = false;

    std::vector<IROperand> vregs_as_dest;
    std::vector<IROperand> vregs_as_src;

    //list all vregs used as dest and as src
    for (const auto& func : m_prog.functions)
    {
        for (const auto&[name, instructions] : func.blocks)
        {
            for (const auto& instr : instructions)
            {
                if (auto it = std::ranges::find(vregs_as_dest, instr.dest);
                    instr.dest.type == IROperand::VirtualReg && it == vregs_as_dest.cend())
                    vregs_as_dest.push_back(instr.dest);
                if (auto it = std::ranges::find(vregs_as_src, instr.src1);
                    instr.src1.type == IROperand::VirtualReg && it == vregs_as_src.cend())
                    vregs_as_src.push_back(instr.src1);
                if (auto it = std::ranges::find(vregs_as_src, instr.src2);
                    instr.src2.type == IROperand::VirtualReg && it == vregs_as_src.cend())
                    vregs_as_src.push_back(instr.src2);
            }
        }
    }

    //needs a change
    if (vregs_as_src.size() != vregs_as_dest.size())
    {
        //we can assume that vregs_as_dest has strictly more elements than vregs_as_src
        //we can also assume that everything in vregs_as_src is also contained in dests vector
        changed = true;
        for (const auto& src : vregs_as_src)
        {
            vregs_as_dest.erase(std::ranges::find(vregs_as_dest, src));
        }

        //now vregs_as_dest only contains those vregs that are only ever used as dests
        for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        {
            for (auto [index_b, block] : std::views::enumerate(func.blocks))
            {
                for (auto [index_i, instr] : std::views::enumerate(block.instructions))
                {
                    if (std::ranges::find(vregs_as_dest, instr.dest) != vregs_as_dest.cend())
                    {
                        m_prog.functions.at(index_f).blocks.at(index_b).instructions.erase(
                            m_prog.functions.at(index_f).blocks.at(index_b).instructions.begin() + index_i);
                    }
                }
            }
        }
    }

    return changed;
}


