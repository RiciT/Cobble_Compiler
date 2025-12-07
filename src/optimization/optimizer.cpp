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
        changed |= reassignment_elimination();
        changed |= constant_propagation_block();
        changed |= constant_folding();
        changed |= unreachable_code_elimination();
        changed |= empty_block_removal();
        changed |= algebraic_reduction();
        changed |= dead_code_elimination();
        //only start coalescing if the basic block opts are already done
        if (!changed) changed |= coalescing();
        //and all other optimizations
    }
}

//change breaks to reverse iterations
#define curr_instrs m_prog.functions.at(index_f).blocks.at(index_b).instructions
#define curr_blocks m_prog.functions.at(index_f).blocks

//these are logiaclly not const since it modifies the referenced member m_prog
// ReSharper disable once CppMemberFunctionMayBeConst
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
    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                if (auto it = std::ranges::find(vregs_as_dest, instr.dest);
                    instr.dest.type == IROperand::VirtualReg && it == vregs_as_dest.cend() &&
                    (arithmetic_ops.contains(instr.opcode) || instr.opcode == IROpcode::COPY))
                    vregs_as_dest.push_back(instr.dest);
                if (auto it = std::ranges::find(vregs_as_src, instr.src1);
                    instr.src1.type == IROperand::VirtualReg && it == vregs_as_src.cend())
                    vregs_as_src.push_back(instr.src1);
                if (auto it = std::ranges::find(vregs_as_src, instr.src2);
                    instr.src2.type == IROperand::VirtualReg && it == vregs_as_src.cend())
                    vregs_as_src.push_back(instr.src2);
            }

    //needs a change
    if (vregs_as_src.size() != vregs_as_dest.size())
    {
        //we can assume that vregs_as_dest has strictly more elements than vregs_as_src
        //we can also assume that everything in vregs_as_src is also contained in dests vector
        changed = true;
        for (const auto& src : vregs_as_src)
            vregs_as_dest.erase(std::ranges::find(vregs_as_dest, src));

        //now vregs_as_dest only contains those vregs that are only ever used as dests
        for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
            for (auto [index_b, block] : std::views::enumerate(func.blocks))
                for (auto [index_i, instr] : std::views::enumerate(block.instructions))
                    if (std::ranges::find(vregs_as_dest, instr.dest) != vregs_as_dest.cend())
                    {
                        //this is a really ugly solution but for now it will do
                        //later ill implement a reversing scheme
                        curr_instrs.erase(curr_instrs.begin() + index_i);
                        break; //TODO change this to something sensible
                    }
    }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::constant_propagation_block()
{
    bool changed = false;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                //only do smth if its of a form tX = Int_Lit
                if (instr.opcode != IROpcode::COPY || instr.src1.type != IROperand::IntLiteral)
                    continue;
                for (int i = index_i + 1; i < block.instructions.size(); i++)
                {
                    //break if the reg is reassigned
                    if (block.instructions.at(i).dest == instr.dest) break;
                    //change from reg to the const
                    if (block.instructions.at(i).src1 == instr.dest)
                    {
                        changed = true;
                        curr_instrs.at(i).src1 = instr.src1;
                    }
                    if (block.instructions.at(i).src2 == instr.dest)
                    {
                        changed = true;
                        curr_instrs.at(i).src2 = instr.src1;
                    }
                }
            }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::constant_folding()
{
    bool changed = false;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                if (!arithmetic_ops.contains(instr.opcode) ||
                    instr.src1.type != IROperand::IntLiteral ||
                    instr.src2.type != IROperand::IntLiteral) continue;
                changed = true;
                curr_instrs.at(index_i) = { IROpcode::COPY, instr.dest,
                    IROperand::make_lit(arithmetic_ops.at(instr.opcode)(instr.src1.val_id, instr.src2.val_id)) };
            }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::unreachable_code_elimination()
{
    bool changed = false;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                if (instr.opcode != IROpcode::GOTO) continue;
                int label_index = curr_instrs.size(); //init to the end to ensure if we end before a label we still remove anything unneccessary
                for (int i = index_i + 1; i < block.instructions.size(); i++)
                {
                    if (curr_instrs.at(i).opcode == IROpcode::LABEL)
                    { label_index = i; break; }
                }
                if (index_i + 1 != label_index)
                {
                    curr_instrs.erase(curr_instrs.begin() + index_i + 1, curr_instrs.begin() + label_index);
                    changed = true;
                    break;
                }
            }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::empty_block_removal()
{
    bool changed = false;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
        {
            if (block.instructions.empty())
            {
                curr_blocks.erase(curr_blocks.begin() + index_b);
                if (index_b <= func.main_control_flow_index) m_prog.functions.at(index_f).main_control_flow_index--;
                changed = true;
                break; //TODO change this to something sensible
            }
        }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::algebraic_reduction()
{
    bool changed = false;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                //getting rid of identity operations
                if (instr.opcode == IROpcode::ADD || instr.opcode == IROpcode::SUB)
                {
                    if (instr.src1 != IROperand::make_lit(0) && instr.src2 != IROperand::make_lit(0)) continue;
                    changed = true;
                    curr_instrs.at(index_i) = {IROpcode::COPY, instr.dest, instr.src1 == IROperand::make_lit(0) ? instr.src2 : instr.src1};
                }
                if (instr.opcode == IROpcode::MUL || instr.opcode == IROpcode::DIV)
                {
                    if (instr.src1 == IROperand::make_lit(1) || instr.src2 == IROperand::make_lit(1))
                    {
                        changed = true;
                        curr_instrs.at(index_i) = {IROpcode::COPY, instr.dest, instr.src1 == IROperand::make_lit(1) ? instr.src2 : instr.src1};
                    }
                }
                //getting rid of multip by zero or two
                if (instr.opcode == IROpcode::MUL && !changed)
                {
                    if (instr.src1 == IROperand::make_lit(0) || instr.src2 == IROperand::make_lit(0))
                    {
                        changed = true;
                        curr_instrs.at(index_i) = {IROpcode::COPY, instr.dest, IROperand::make_lit(0)};
                    }

                    if (instr.src1 == IROperand::make_lit(2) || instr.src2 == IROperand::make_lit(2))
                    {
                        changed = true;
                        const auto vreg = instr.src1 == IROperand::make_lit(2) ? instr.src2 : instr.src1;
                        curr_instrs.at(index_i) = {IROpcode::ADD, instr.dest, vreg, vreg};
                    }
                }
            }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::reassignment_elimination()
{
    bool changed = false;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                if (instr.dest.type != IROperand::VirtualReg) continue;
                //these are the opcodes that assign vars
                if (!arithmetic_ops.contains(instr.opcode) && instr.opcode != IROpcode::COPY) continue;

                //these are for continued searching so that we have to iterate less times
                IROperand vreg = instr.dest;
                int pivot = index_i;

                for (int i = index_i + 1; i < curr_instrs.size(); i++)
                {
                    if (curr_instrs.at(i).src1 == vreg || curr_instrs.at(i).src2 == vreg) break;
                    if (curr_instrs.at(i).dest == vreg)
                    {
                        changed = true;
                        curr_instrs.erase(curr_instrs.begin() + pivot);

                        pivot = i; //for continued searching

                        break; //TODO change this to something sensible
                    }
                }
                if (pivot != index_i) break; //TODO change this to something sensible
            }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::dead_code_elimination()
{
    bool changed = false;

    std::vector<std::string> labels_to_remove;
    std::vector<std::string> labels_used = {"_start"};
    std::vector<std::string> all_labels;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                if (instr.opcode == IROpcode::GOTRUE)
                {
                    if (const auto it = std::ranges::find(labels_to_remove, instr.dest.label);
                        it == labels_to_remove.cend() && instr.src1 == IROperand::make_lit(0))
                        labels_to_remove.push_back(instr.dest.label);
                    else if (it != labels_to_remove.cend() && instr.src1 != IROperand::make_lit(0))
                        labels_to_remove.erase(it);
                }
                if (instr.opcode == IROpcode::GOTRUE || instr.opcode == IROpcode::GOTO)
                {
                    if (const auto it = std::ranges::find(labels_used, instr.dest.label); it == labels_used.cend())
                        labels_used.push_back(instr.dest.label);
                    else
                        labels_used.erase(it);
                }
                if (instr.opcode == IROpcode::LABEL)
                {
                    if (const auto it = std::ranges::find(all_labels, instr.dest.label); it == all_labels.cend())
                        all_labels.push_back(instr.dest.label);
                    else
                        all_labels.erase(it);
                }
            }

    if (!labels_to_remove.empty() || all_labels.size() != labels_used.size())
    {
        changed = true;
        //somehow need to loop through labels_to_remove
        //remove basic blocks starting with the label
        //remove all GOTRUE instances with that label
        for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
            for (auto [index_b, block] : std::views::enumerate(func.blocks))
            {
                if (block.instructions.front().opcode == IROpcode::LABEL &&
                    std::ranges::find(labels_to_remove, block.instructions.front().dest.label) != labels_to_remove.cend())
                {
                    curr_blocks.erase(curr_blocks.begin() + index_b);
                    if (index_b <= func.main_control_flow_index) m_prog.functions.at(index_f).main_control_flow_index--;
                    break; //TODO change this to something sensible
                    continue;
                }
                for (auto [index_i, instr] : std::views::enumerate(block.instructions))
                {
                    if (instr.opcode == IROpcode::GOTRUE && std::ranges::find(labels_to_remove, instr.dest.label) != labels_to_remove.cend())
                    {
                        curr_instrs.erase(curr_instrs.begin() + index_i);
                        break; //TODO change this to something sensible
                    }
                    if (instr.opcode == IROpcode::LABEL && std::ranges::find(labels_used, instr.dest.label) == labels_used.cend())
                    {
                        curr_instrs.erase(curr_instrs.begin() + index_i);
                        break; //TODO change this to something sensible
                    }
                }

            }
    }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::coalescing_single_jump_labels(const long index_f, IRFunction& func)
{
    bool changed = false;
    std::unordered_map<std::string, std::tuple<int, bool, std::vector<std::pair<int, int>>>> label_instance_count;
    for (auto [index_b, block] : std::views::enumerate(func.blocks))
    {
        for (auto [index_i, instr] : std::views::enumerate(block.instructions))
        {
            if (instr.opcode == IROpcode::GOTO || instr.opcode == IROpcode::GOTRUE)
            {
                auto& [jump_count, jump_is_uncond, jump_pos] = label_instance_count[instr.dest.label];
                jump_count++;
                jump_is_uncond = instr.opcode == IROpcode::GOTO ? true : instr.src1 == IROperand::make_lit(1);
                jump_pos.push_back(std::pair {index_b, index_i});
            }
            //since this is also basically a fallthrough case
            if (instr.opcode == IROpcode::LABEL && curr_instrs.front() != instr)
            { auto& [c, u, p] = label_instance_count[instr.dest.label]; c++; }
        }

        //only do this if we are not on the last block
        if (func.blocks[index_b] == func.blocks.at(func.blocks.size() - 1)) break;
        //also add to label_count if the previous block falls_through to it
        //we can fall through if there is an exit
        if (curr_instrs.back().opcode == IROpcode::EXIT) continue;
        //if either index_b or index_b + 1 is greater than func.main_control_flow_index
        //falling through has no meaning since we have to return to the main control flow at the end of every block outside it
        if (index_b > func.main_control_flow_index) continue;
        //if the next block's first instr is a label and this blocks last instruction is not a GO op or
        //a GO op with the same label we know we will fall through
        auto b_plus_one_first_instr = func.blocks[index_b + 1].instructions.front();
        if (b_plus_one_first_instr.opcode == IROpcode::LABEL)
        { auto& [c, u, p] = label_instance_count[b_plus_one_first_instr.dest.label]; c++; }
        else continue;
        //dont add if there is an uncond GO op to the label
        if ((curr_instrs.back().opcode == IROpcode::GOTO && curr_instrs.back().dest.label == b_plus_one_first_instr.dest.label) ||
            (curr_instrs.back().opcode == IROpcode::GOTRUE && curr_instrs.back().src1 == IROperand::make_lit(1)
                && curr_instrs.back().dest.label == b_plus_one_first_instr.dest.label))
        { auto& [c, u, p] = label_instance_count[b_plus_one_first_instr.dest.label]; c--; }
    }
    if (!label_instance_count.empty())
    {
        for (auto [label, count] : label_instance_count)
        {
            //only do something if the jump is unconditional and single instanced
            if (!(std::get<0>(count) <= 1 && std::get<1>(count))) continue;
            std::pair label_index = {-1, -1};
            for (auto [index_b, block] : std::views::enumerate(func.blocks))
            {
                for (auto [index_i, instr] : std::views::enumerate(block.instructions))
                    if (instr.opcode == IROpcode::LABEL && instr.dest.label == label)
                    { label_index = {index_b, index_i}; break; }
                if (label_index != std::pair {-1, -1}) break;
            }
            //label not in current function so for now we ignore
            if (label_index == std::pair {-1, -1}) continue;

            changed = true;
            #define f_inst curr_blocks.at(std::get<2>(count).at(0).first).instructions
            #define l_inst curr_blocks.at(label_index.first).instructions
            f_inst.insert(f_inst.end() - 1, std::make_move_iterator(l_inst.begin() + 1), std::make_move_iterator(l_inst.end()));
            f_inst.erase(f_inst.end() - 1);
            #undef f_inst
            #undef l_inst
            curr_blocks.erase(curr_blocks.begin() + label_index.first);
            if (label_index.first <= func.main_control_flow_index) m_prog.functions.at(index_f).main_control_flow_index--;

            break; //TODO same
        }
    }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::coalescing()
{
    bool changed = false;

    m_prog.debug_print();std::cout << "\n";

    //this handles single instanced unconditionally jumped to labels and fallthroughs
    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
    {
        #pragma region single jump labels
        changed |= coalescing_single_jump_labels(index_f, func);
        #pragma endregion
        #pragma region fallthrough blocks
        //this handles fallthrough blocks
        //iterate to size - 1 since we are checking adjacent blocks
        for (size_t index_b = 0; index_b < func.blocks.size() - 1; index_b++)
        {
            auto&[_, pred_instructions] = func.blocks[index_b];
            auto&[__, succ_instructions] = func.blocks[index_b+1];

            bool can_change = true;
            //if predecessor does not have any goto or gotrues and successor any labels we can safely fall through
            for (const auto instr : pred_instructions)
                can_change &= instr.opcode != IROpcode::GOTO && instr.opcode != IROpcode::GOTRUE;
            for (const auto instr : succ_instructions)
                can_change &= instr.opcode != IROpcode::LABEL;

            if (can_change)
            {
                curr_blocks.at(index_b).instructions.insert(curr_blocks.at(index_b).instructions.end(),
                    std::make_move_iterator(curr_blocks.at(index_b+1).instructions.begin()),
                    std::make_move_iterator(curr_blocks.at(index_b+1).instructions.end()));
                curr_blocks.erase(curr_blocks.begin() + index_b + 1);
                if (index_b + 1 <= func.main_control_flow_index) m_prog.functions.at(index_f).main_control_flow_index--;
                changed = true;
                break; //TODO same as others
            }
        }
        #pragma endregion
    }

    return changed;
}

#undef curr_instrs
#undef curr_blocks