#include "generator.hpp"

AsmGenerator::AsmGenerator(IRProgram prog)
    : m_prog(std::move(prog))
{
}

std::string AsmGenerator::generate_program()
{
    for (auto& func: m_prog.functions)
        generate_function(func);
    return m_emitter.build_output();
}

std::string AsmGenerator::vreg_stack_loc(size_t vreg_id)
{
    return aop[QWORD] + " [" + regs[rsp] + " - " + std::to_string((vreg_id + 1) * 8) + "]";
}

std::string AsmGenerator::to_x86_operand(const IROperand& opnd)
{
    if (opnd.type == IROperand::IntLiteral) return std::to_string(opnd.val_id);
    if (opnd.type == IROperand::VirtualReg) return vreg_stack_loc(opnd.val_id);
    if (opnd.type == IROperand::Label) return opnd.label;
    return "0";
}

void AsmGenerator::generate_function(const IRFunction& func)
{
    for(const auto& block : func.blocks)
        for(const auto& instr : block.instructions)
        {
            struct load_to_reg {
                AsmEmitter& m_emitter;
                AsmGenerator* gen;
                void operator()(const IROperand& src, int reg) const
                {
                    m_emitter.emit(aop[MOV], regs[static_cast<Reg>(reg)], gen->to_x86_operand(src));
                };
                void operator()(const IROperand& src, Reg reg) const
                {
                    m_emitter.emit(aop[MOV], regs[reg], gen->to_x86_operand(src));
                };
            };
            struct store_from_reg {
                AsmEmitter& m_emitter;
                AsmGenerator* gen;
                void operator()(int reg, const IROperand& src) const
                {
                    m_emitter.emit(aop[MOV], gen->to_x86_operand(src), regs[static_cast<Reg>(reg)]);
                };
                void operator()(Reg reg, const IROperand& src) const
                {
                    m_emitter.emit(aop[MOV], gen->to_x86_operand(src), regs[reg]);
                };
            };
            load_to_reg load_src_to_reg(m_emitter,this);
            store_from_reg store_reg_to_dest(m_emitter,this);

            //we need to track whats in each reg so we avoid unnecessary movs
            //we should also only write to memory from register if the register would get overwritten
            switch(const auto oc = instr.opcode) {
                case IROpcode::COPY: {
                    load_src_to_reg(instr.src1, rax);
                    store_reg_to_dest(rax, instr.dest);
                    break;
                }
                case IROpcode::ADD:
                case IROpcode::SUB: {
                    load_src_to_reg(instr.src1, rax);
                    if (instr.src2.type != IROperand::IntLiteral) load_src_to_reg(instr.src2, rbx);
                    m_emitter.emit(op[oc == IROpcode::ADD ? IROpcode::ADD : IROpcode::SUB], regs[rax],
                        instr.src2.type == IROperand::IntLiteral ? std::to_string(instr.src2.val_id) : regs[rbx]);
                    store_reg_to_dest(rax, instr.dest);
                    break;
                }
                case IROpcode::MUL:
                case IROpcode::DIV: {
                    load_src_to_reg(instr.src1, rax);
                    load_src_to_reg(instr.src2, rbx);
                    if (oc == IROpcode::DIV) m_emitter.emit(aop[XOR], regs[rdx], regs[rdx]);
                    m_emitter.emit(op[oc == IROpcode::MUL ? IROpcode::MUL : IROpcode::DIV], regs[rbx]);
                    store_reg_to_dest(rax, instr.dest);
                    break;
                }
                case IROpcode::EQ:
                case IROpcode::NEQ:
                case IROpcode::LT:
                case IROpcode::GT:
                case IROpcode::LTE:
                case IROpcode::GTE: {
                    load_src_to_reg(instr.src1, rax);
                    load_src_to_reg(instr.src2, rbx);
                    m_emitter.emit(aop[CMP], regs[rax], regs[rbx]);
                    m_emitter.emit(op[oc], regs[al]);
                    m_emitter.emit(aop[MOVZX], regs[rax], regs[al]);
                    store_reg_to_dest(rax, instr.dest);
                    break;
                }
                case IROpcode::LABEL: {
                    //for now until we change to fully generating from the ir - will need to change asm_emitter
                    if (instr.dest.label != "_start") m_emitter.emit_label(instr.dest.label);
                    break;
                }
                case IROpcode::GOTO: {
                    m_emitter.emit(op[IROpcode::GOTO], instr.dest.label);
                    break;
                }
                case IROpcode::GOTRUE: {
                    load_src_to_reg(instr.src1, rax);
                    m_emitter.emit(aop[TEST], regs[rax], regs[rax]);
                    m_emitter.emit(op[IROpcode::GOTRUE], instr.dest.label);
                    break;
                }
                case IROpcode::EXIT: {
                    load_src_to_reg(IROperand::make_lit(EXIT_CODE), rax);
                    load_src_to_reg(instr.src1, rdi);
                    m_emitter.emit(aop[SYSCALL]);
                    break;
                }
                case IROpcode::PRINT: {
                    load_src_to_reg(instr.src1, rax);
                    m_emitter.emit_comment("PRINT value in RAX (TODO: Implement Int-to-String)");
                    break;
                }
                default: {
                    m_emitter.emit_comment("TODO: Implement Opcode");
                    break;
                }
            }
        }
    //will need a ret statement if we are generating functions and not main
}