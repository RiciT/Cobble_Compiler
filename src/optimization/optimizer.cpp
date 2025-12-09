#include "optimizer.hpp"

#include <vector>
#include <algorithm>
#include <ranges>
#include <set>

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
        //only start func-wide optimizations if the basic block opts are already done
        if (!changed) changed |= coalescing();
        if (!changed) changed |= jump_target_merging();
        //global optimizations
        if (!changed) changed |= copy_propagation();
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

    std::unordered_map<std::string, bool> labels_to_remove;
    std::vector<std::string> labels_used = {"_start"};
    std::vector<std::string> all_labels;

    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                if (instr.opcode == IROpcode::GOTRUE)
                {
                    if (!labels_to_remove.contains(instr.dest.label) && instr.src1 == IROperand::make_lit(0))
                        labels_to_remove[instr.dest.label] = true;
                    else if (labels_to_remove.contains(instr.dest.label) && instr.src1 != IROperand::make_lit(0))
                        labels_to_remove[instr.dest.label] = false;
                }
                if (instr.opcode == IROpcode::GOTRUE || instr.opcode == IROpcode::GOTO)
                    if (const auto it = std::ranges::find(labels_used, instr.dest.label); it == labels_used.cend())
                        labels_used.push_back(instr.dest.label);
                if (instr.opcode == IROpcode::LABEL)
                    if (const auto it = std::ranges::find(all_labels, instr.dest.label); it == all_labels.cend())
                        all_labels.push_back(instr.dest.label);
            }

    if (labels_to_remove.empty() && all_labels.size() == labels_used.size()) return changed;

    changed = true;
    //somehow need to loop through labels_to_remove
    //remove basic blocks starting with the label
    //remove all GOTRUE instances with that label
    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
        {
            if (block.instructions.front().opcode == IROpcode::LABEL &&
                labels_to_remove.contains(block.instructions.front().dest.label) &&
                labels_to_remove[block.instructions.front().dest.label])
            {
                curr_blocks.erase(curr_blocks.begin() + index_b);
                if (index_b <= func.main_control_flow_index) m_prog.functions.at(index_f).main_control_flow_index--;
                break; //TODO change this to something sensible
                continue;
            }
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
            {
                if (instr.opcode == IROpcode::GOTRUE &&
                    labels_to_remove.contains(instr.dest.label) &&
                    labels_to_remove[instr.dest.label])
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

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::coalescing_single_jump_labels(const long index_f, IRFunction& func)
{
    bool changed = false;
    // { name, { count, is_unconditional, positions { block_index, instr_index } } }
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
    if (label_instance_count.empty()) return changed;
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

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::coalescing()
{
    bool changed = false;

    //this handles single instanced unconditionally jumped to labels and fallthroughs
    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
    {
        changed |= coalescing_single_jump_labels(index_f, func);
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
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::jump_target_merging()
{
    bool changed = false;

    //vector of a tuple of identical blocks' name and block index and the function index
    std::vector<std::tuple<int, std::pair<std::string, int>, std::pair<std::string, int>>> ident_block_label_pairs;
    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b_1, block_1] : std::views::enumerate(func.blocks))
        {
            std::string flows_into_label = "";

            //if the front is not a label
            if (block_1.instructions.front().opcode != IROpcode::LABEL) continue;
            //and the back is not an unconditional jump we continue
            //or if it flows into a label save that label
            if (block_1.instructions.back().opcode != IROpcode::GOTO &&
                !(block_1.instructions.back().opcode == IROpcode::GOTRUE &&
                    block_1.instructions.back().src1 == IROperand::make_lit(1)))
            {
                if (index_b_1 + 1 < func.blocks.size() && func.blocks[index_b_1 + 1].instructions.front().opcode == IROpcode::LABEL)
                    flows_into_label = func.blocks[index_b_1 + 1].instructions.front().dest.label;
                else
                    continue;
            }
            auto label1 = block_1.instructions.front().dest.label;

            for (size_t index_b_2 = index_b_1 + 1; index_b_2 < func.blocks.size(); index_b_2++)
            {
                auto block_2 = func.blocks.at(index_b_2);
                //if the blocks are not the same size or block_1 can be one shorter incase it flows into a label
                if (block_1.instructions.size() != block_2.instructions.size() &&
                    block_1.instructions.size() + 1 != block_2.instructions.size()) continue;
                //and the front is not a label
                if (block_2.instructions.front().opcode != IROpcode::LABEL) continue;
                //and the back is not an unconditional jump we continue
                if (block_2.instructions.back().opcode != IROpcode::GOTO &&
                    !(block_2.instructions.back().opcode == IROpcode::GOTRUE &&
                        block_2.instructions.back().src1 == IROperand::make_lit(1))) continue;
                auto label2 = block_2.instructions.front().dest.label;

                bool isSame = true;
                //start at one since label will not be the same
                for (size_t index_i = 1; index_i < block_2.instructions.size(); index_i++)
                {
                    if (block_1.instructions.size() == index_i &&
                        (block_2.instructions.at(index_i) == IRInstruction {IROpcode::GOTO, IROperand::make_label(flows_into_label)} ||
                            block_2.instructions.at(index_i) == IRInstruction {IROpcode::GOTRUE, IROperand::make_label(flows_into_label), IROperand::make_lit(1)}))
                        break;
                    if (block_1.instructions.at(index_i) != block_2.instructions.at(index_i))
                    { isSame = false; break; }
                }
                if (isSame) ident_block_label_pairs.push_back({ index_f, std::pair { label1, index_b_1 }, std::pair { label2, index_b_2 } });
            }
        }

    //now we have the label pairs in ident_block_label_pairs
    if (ident_block_label_pairs.empty()) return changed;

    std::vector<std::pair<int, int>> deletion_indeces;
    //we still need to check if any of the blocks are being fallen into - if at least one is safe to delete then we should do just that
    for (auto [index_f, lab1, lab2] : ident_block_label_pairs)
    {
        bool fall_through_block_1 = true;
        bool fall_through_block_2 = true;

        if (curr_blocks.at(lab1.second) == curr_blocks.front()) fall_through_block_1 = false;
        else if (curr_blocks.at(lab2.second) == curr_blocks.front()) fall_through_block_2 = false;
        if (fall_through_block_1 &&
            (curr_blocks.at(lab1.second - 1).instructions.back().opcode == IROpcode::GOTO ||
            (curr_blocks.at(lab1.second - 1).instructions.back().opcode == IROpcode::GOTRUE &&
                    curr_blocks.at(lab1.second - 1).instructions.back().src1 == IROperand::make_lit(1)))) fall_through_block_1 = false;
        if (fall_through_block_2 &&
            (curr_blocks.at(lab2.second - 1).instructions.back().opcode == IROpcode::GOTO ||
            (curr_blocks.at(lab2.second - 1).instructions.back().opcode == IROpcode::GOTRUE &&
                    curr_blocks.at(lab2.second - 1).instructions.back().src1 == IROperand::make_lit(1)))) fall_through_block_2 = false;

        //list deletable block indeces and change labels
        if (const int index_to_del = !fall_through_block_2 ? 1 : !fall_through_block_1 ? 0 : -1; index_to_del != -1)
        {
            changed = true;
            //delete the block we dont need
            deletion_indeces.push_back({index_f, std::vector { lab1, lab2 }[index_to_del].second});
            //replace every instance of goto/gotrue labels of the deleted block's label with the other's label
            for (auto [index_b, block] : std::views::enumerate(m_prog.functions.at(index_f).blocks))
                for (auto [index_i, instr] : std::views::enumerate(block.instructions))
                    if ((instr.opcode == IROpcode::GOTO || instr.opcode == IROpcode::GOTRUE) &&
                        instr.dest == IROperand::make_label(std::vector { lab1, lab2 }[index_to_del].first))
                        curr_instrs.at(index_i).dest.label = std::vector { lab2, lab1 }[index_to_del].first;
        }
    }

    //if we need to delete any blocks - isnt even needed per se since unreachable
    //code deletion would get them eventually but its nicer this way
    if (!deletion_indeces.empty())
    {
        //sort in descending order by the block index and get rid of duplicates
        std::ranges::sort(deletion_indeces, [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
            return a.second > b.second;
        });
        const std::vector<std::pair<int, int>>::const_iterator last = std::ranges::unique(deletion_indeces).begin();
        deletion_indeces.erase(last, deletion_indeces.end());

        for (int i = 0; i < deletion_indeces.size(); i++)
        {
            const int index_f = deletion_indeces[i].first;
            curr_blocks.erase(curr_blocks.begin() + deletion_indeces[i].second);
        }
    }

    return changed;
}
// ReSharper disable once CppMemberFunctionMayBeConst
bool IROptimizer::copy_propagation()
{
    bool changed = false;

    //register  - regdata of copy - number of arithm
    std::unordered_map<int, std::pair<std::vector<IROperand>, int>> all_registers;
    //for now only propagate temps - once assigned
    for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
        for (auto [index_b, block] : std::views::enumerate(func.blocks))
            for (auto [index_i, instr] : std::views::enumerate(block.instructions))
                if (instr.opcode == IROpcode::COPY || arithmetic_ops.contains(instr.opcode))
                { auto& [d, a] = all_registers[instr.dest.val_id];
                    if (instr.opcode == IROpcode::COPY) { d.push_back(instr.src1); } else a++; }

    if (all_registers.empty()) return false;

    //indexf - indexb - indexi
    std::vector<std::tuple<int, int, int>> erase_coords;
    for (auto [reg, occ] : all_registers)
    {
        if (occ.first.size() != 1 || occ.second != 0) continue;
        changed = true;

        for (auto [index_f, func] : std::views::enumerate(m_prog.functions))
            for (auto [index_b, block] : std::views::enumerate(func.blocks))
                for (auto [index_i, instr] : std::views::enumerate(block.instructions))
                {
                    if (instr.opcode == IROpcode::COPY && instr.dest == IROperand::make_reg(reg))
                    { erase_coords.push_back({index_f, index_b, index_i}); continue; }
                    if (instr.src1 == IROperand::make_reg(reg))
                        curr_instrs[index_i].src1 = occ.first.front();
                    if (instr.src2 == IROperand::make_reg(reg))
                        curr_instrs[index_i].src2 = occ.first.front();

                }
    }

    //it cannot be empty but check just to be sure
    if (erase_coords.empty()) return changed;

    //erase_coords already in ascending order since we assign it in a forwards loop
    for (int i = erase_coords.size() - 1; i >= 0; i--)
    {
        const auto index_f = std::get<0>(erase_coords[i]);
        const auto index_b = std::get<1>(erase_coords[i]);
        const auto index_i = std::get<2>(erase_coords[i]);
        curr_instrs.erase(curr_instrs.begin() + index_i);
    }

    return changed;
}


#undef curr_instrs
#undef curr_blocks