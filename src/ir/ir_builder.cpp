#include <cassert>
#include <ranges>

#include "ir_builder.hpp"

IRBuilder::IRBuilder(NodeProgram prog)
    : m_prog(std::move(prog))
{
}

IRProgram IRBuilder::generate_ir()
{
    // 1. Create '_start' function (Entry Point)
    IRFunction main_func;
    main_func.name = "_start"; 
    m_result.functions.push_back(main_func);
    
    // Set context to main
    m_current_func = &m_result.functions.back();
    m_current_func->blocks.push_back(IRBasicBlock{.name = "entry"});
    m_current_block = &m_current_func->blocks.back();

    push_scope(); // Global scope

    // 2. Separate function definitions from standard statements
    // We must generate all functions *outside* the flow of the main function.
    std::vector<const NodeStmt*> standard_stmts;
    
    for (const auto* stmt : m_prog.stmts) {
        if (std::holds_alternative<NodeStmtFunc*>(stmt->stmt)) {
            // Save context
            IRFunction* saved_func = m_current_func;
            IRBasicBlock* saved_block = m_current_block;
            // Note: We copy scopes to allow functions to potentially see globals, 
            // though strict separation is often safer. For now, we clear locals logic below.
            auto saved_scopes = m_scopes; 

            // --- Generate Function ---
            const auto func_node = std::get<NodeStmtFunc*>(stmt->stmt);
            
            IRFunction new_func;
            new_func.name = "func_" + std::string(func_node->ident.value.value());
            m_result.functions.push_back(new_func);
            m_current_func = &m_result.functions.back();
            m_current_func->blocks.push_back(IRBasicBlock{.name = "entry"});
            m_current_block = &m_current_func->blocks.back();
            
            // Start fresh scope for function (isolating locals)
            m_scopes.clear(); 
            push_scope();

            // Handle Parameters
            if (func_node->params.has_value()) {
                // In IR, we treat parameters as pre-existing virtual registers.
                // The Backend will handle mapping them to physical registers (RDI, RSI...) or stack slots.
                for (const auto& param : func_node->params.value()) {
                    IROperand param_reg = create_vreg();
                    m_current_func->params.push_back(param_reg);
                    add_var(std::string(param.ident.value.value()), {param_reg, false, 0});
                }
            }

            generate_scope(func_node->scope);
            
            // Ensure implicit return 0 for void functions or missing return paths
            if (m_current_block->instructions.empty() || m_current_block->instructions.back().opcode != IROpcode::RET) {
                emit(IRInstruction::make_ret(IROperand::make_lit(0)));
            }

            // Restore context
            m_current_func = saved_func;
            m_current_block = saved_block;
            m_scopes = saved_scopes;
        } 
        else {
            standard_stmts.push_back(stmt);
        }
    }

    // 3. Generate Main body
    for (const auto* stmt : standard_stmts) {
        generate_stmt(stmt);
    }
    
    // Default exit for main
    emit(IRInstruction{IROpcode::EXIT, {}, IROperand::make_lit(0), {}});

    pop_scope();
    return m_result;
}

//statement generation
void IRBuilder::generate_scope(const NodeScope* scope) {
    push_scope();
    for (const auto* stmt : scope->stmts) {
        generate_stmt(stmt);
    }
    pop_scope();
}

void IRBuilder::generate_stmt(const NodeStmt* stmt) {
    struct StmtVisitor {
        IRBuilder& builder;

        void operator()(const NodeStmtExit* s) {
            IROperand val = builder.generate_expr(s->expr);
            builder.emit(IRInstruction{IROpcode::EXIT, {}, val, {}});
        }
        
        void operator()(const NodeStmtPrint* s) {
            IROperand val = builder.generate_expr(s->expr);
            builder.emit(IRInstruction{IROpcode::PRINT, {}, val, {}});
        }
        
        void operator()(const NodeStmtDef* s) {
            IROperand vreg = builder.create_vreg();
            VarInfo info { vreg, s->type.is_array, 0 };
            
            if (s->type.is_array) {
                 if (s->array_size_expr.has_value()) {
                     // Evaluate size constant. 
                     // In a robust compiler, this should handle runtime sizes via `alloca`.
                     // Here we assume simple stack reservation by the backend based on this size.
                     // We calculate bytes: size * 8
                     // But for IR, we just store the count or bytes. Let's store Count.
                     // Note: We need to evaluate the expression to get the number.
                     // The type checker guarantees this is constant, but the AST holds it as an Expr.
                     // For now, we will trust the Backend to handle the allocation size if passed as meta,
                     // OR we emit an ALLOCA instruction.
                     // Since `ir.hpp` doesn't have ALLOCA, we rely on the Backend using the VarInfo `array_size`.
                     
                     // Limitation: This assumes we can evaluate the literal here.
                     // Ideally, NodeStmtDef would have a pre-calculated size from analysis.
                     info.array_size = 8; // Default fallback if extraction fails (TODO: Extract literal)
                 }
            } 
            else if (s->expr.has_value()) {
                IROperand val = builder.generate_expr(s->expr.value());
                builder.emit(IRInstruction::make_mov(vreg, val));
            } else {
                 // Zero initialize
                 builder.emit(IRInstruction::make_mov(vreg, IROperand::make_lit(0)));
            }
            
            builder.add_var(std::string(s->ident.value.value()), info);
        }
        
        void operator()(const NodeStmtAssign* s) {
            VarInfo* var = builder.find_var(std::string(s->ident.value.value()));
            assert(var && "Variable not found (Analysis phase failed?)");
            
            IROperand val = builder.generate_expr(s->expr);
            builder.emit(IRInstruction::make_mov(var->reg, val));
        }
        
        void operator()(const NodeStmtIf* s) {
            IROperand cond = builder.generate_expr(s->expr);
            
            std::string label_next = builder.create_label(); // Else or End
            std::string label_end = builder.create_label();  // Very End
            
            // if (cond == 0) goto next
            builder.emit(IRInstruction::make_cmp(cond, IROperand::make_lit(0)));
            builder.emit(IRInstruction::make_jump(IROpcode::JE, label_next));
            
            // Then block
            builder.generate_scope(s->scope);
            builder.emit(IRInstruction::make_jump(IROpcode::JMP, label_end));
            
            // Next block (ElseIf or Else or End)
            builder.emit(IRInstruction::make_label(label_next));
            
            if (s->ifpred.has_value()) {
                builder.generate_if_predicate(s->ifpred.value(), label_end);
            }
            
            builder.emit(IRInstruction::make_label(label_end));
        }
        
        void operator()(const NodeStmtWhile* s) {
            std::string label_start = builder.create_label();
            std::string label_end = builder.create_label();
            
            builder.emit(IRInstruction::make_label(label_start));
            
            IROperand cond = builder.generate_expr(s->expr);
            builder.emit(IRInstruction::make_cmp(cond, IROperand::make_lit(0)));
            builder.emit(IRInstruction::make_jump(IROpcode::JE, label_end));
            
            builder.generate_scope(s->scope);
            builder.emit(IRInstruction::make_jump(IROpcode::JMP, label_start));
            
            builder.emit(IRInstruction::make_label(label_end));
        }
        
        void operator()(const NodeStmtFunc* s) {
             // Handled in generate_ir pass 2
        }

        void operator()(const NodeStmtFuncCall* s) {
            // Push arguments
            std::vector<IROperand> args;
            if (s->exprs.has_value()) {
                for (auto* expr : s->exprs.value()) {
                    args.push_back(builder.generate_expr(expr));
                }
            }
            
            // Push right-to-left
            for (auto it = args.rbegin(); it != args.rend(); ++it) {
                builder.emit({IROpcode::PUSH, {}, *it, {}});
            }
            
            std::string func_lbl = "func_" + std::string(s->ident.value.value());
            builder.emit(IRInstruction::make_jump(IROpcode::CALL, func_lbl));
            
            // We ignore the return value here as this is a Statement (void context)
        }
        
        void operator()(const NodeStmtReturn* s) {
            if (s->expr.has_value()) {
                IROperand val = builder.generate_expr(s->expr.value());
                builder.emit(IRInstruction::make_ret(val));
            } else {
                builder.emit(IRInstruction::make_ret(IROperand::make_lit(0)));
            }
        }
        
        void operator()(const NodeStmtArrayAssign* s) {
            VarInfo* var = builder.find_var(std::string(s->ident.value.value()));
            IROperand idx = builder.generate_expr(s->index);
            IROperand val = builder.generate_expr(s->value);
            
            // Address = base + index * 8
            IROperand offset = builder.create_vreg();
            builder.emit(IRInstruction::make_binary(IROpcode::MUL, offset, idx, IROperand::make_lit(8)));
            
            // Add offset to base?
            // In stack-based arrays, 'var->reg' usually points to the start or is the RSP offset.
            // For this IR, we assume var->reg holds the Base Address.
            // If the backend allocates it on stack, it must handle the pointer math.
            // We emit a STORE that takes (Base, Offset, Value).
            
            // Explicit address calc:
            IROperand addr = builder.create_vreg();
            builder.emit(IRInstruction::make_binary(IROpcode::ADD, addr, var->reg, offset));
            
            // STORE [addr], val
            // We use the STORE opcode: STORE addr_reg, val
            builder.emit({IROpcode::STORE, {}, addr, val});
        }
        
        void operator()(const NodeScope* s) {
            builder.generate_scope(s);
        }
    };

    std::visit(StmtVisitor{*this}, stmt->stmt);
}

void IRBuilder::generate_if_predicate(const NodeIfPredicate* pred, const std::string& end_label) {
    struct PredVisitor {
        IRBuilder& builder;
        const std::string& end_label;

        void operator()(const NodeIfPredElseIf* elseif) {
            IROperand cond = builder.generate_expr(elseif->expr);
            std::string label_next = builder.create_label(); // Next ElseIf or Else
            
            // if (cond == 0) goto next
            builder.emit(IRInstruction::make_cmp(cond, IROperand::make_lit(0)));
            builder.emit(IRInstruction::make_jump(IROpcode::JE, label_next));
            
            // Block
            builder.generate_scope(elseif->scope);
            builder.emit(IRInstruction::make_jump(IROpcode::JMP, end_label));
            
            builder.emit(IRInstruction::make_label(label_next));
            
            // Recursion
            if (elseif->ifpred.has_value()) {
                builder.generate_if_predicate(elseif->ifpred.value(), end_label);
            }
        }

        void operator()(const NodeIfPredElse* els) {
            builder.generate_scope(els->scope);
            // Fall through to end_label naturally
        }
    };

    std::visit(PredVisitor{*this, end_label}, pred->ifpred);
}

//expression generation
IROperand IRBuilder::generate_expr(const NodeExpr* expr) {
    struct ExprVisitor {
        IRBuilder& builder;
        IROperand operator()(const NodeAtom* atom) { return builder.generate_atom(atom); }
        IROperand operator()(const NodeBinExpr* bin) { return builder.generate_binexpr(bin); }
        IROperand operator()(const NodeFuncCallExpr* call) { 
            // 1. Generate Arguments
            std::vector<IROperand> args;
            if (call->exprs.has_value()) {
                for (auto* e : call->exprs.value()) {
                    args.push_back(builder.generate_expr(e));
                }
            }
            
            // 2. Push Arguments (Reverse order)
            for (auto it = args.rbegin(); it != args.rend(); ++it) {
                builder.emit({IROpcode::PUSH, {}, *it, {}});
            }
            
            // 3. Call
            std::string func_lbl = "func_" + std::string(call->ident.value.value());
            IROperand dest = builder.create_vreg();
            
            // "dest = CALL label"
            // We construct the instruction manually to ensure dest is set
            builder.emit({IROpcode::CALL, dest, IROperand::make_label(func_lbl), {}});
            
            return dest;
        }
    };
    return std::visit(ExprVisitor{*this}, expr->expr);
}

IROperand IRBuilder::generate_binexpr(const NodeBinExpr* bin_expr) {
    struct BinVisitor {
        IRBuilder& builder;
        
        IROperand handle_arith(IROpcode op, NodeExpr* lhs, NodeExpr* rhs) {
            IROperand l = builder.generate_expr(lhs);
            IROperand r = builder.generate_expr(rhs);
            IROperand dest = builder.create_vreg();
            builder.emit(IRInstruction::make_binary(op, dest, l, r));
            return dest;
        }

        IROperand handle_comp(IROpcode jump_op, NodeExpr* lhs, NodeExpr* rhs) {
            // Logic:
            // CMP l, r
            // MOV dest, 1
            // J_condition true_label
            // MOV dest, 0
            // true_label:
            
            // Optimization:
            // CMP l, r
            // MOV dest, 0
            // J_inverse_condition skip
            // MOV dest, 1
            // skip:
            
            IROperand l = builder.generate_expr(lhs);
            IROperand r = builder.generate_expr(rhs);
            IROperand dest = builder.create_vreg();
            std::string label_true = builder.create_label();
            std::string label_end = builder.create_label();
            
            builder.emit(IRInstruction::make_cmp(l, r));
            
            // Jump to 'true' block if condition met
            builder.emit(IRInstruction::make_jump(jump_op, label_true));
            
            // False case
            builder.emit(IRInstruction::make_mov(dest, IROperand::make_lit(0)));
            builder.emit(IRInstruction::make_jump(IROpcode::JMP, label_end));
            
            // True case
            builder.emit(IRInstruction::make_label(label_true));
            builder.emit(IRInstruction::make_mov(dest, IROperand::make_lit(1)));
            
            builder.emit(IRInstruction::make_label(label_end));
            return dest;
        }

        IROperand operator()(const NodeBinExprAdd* e) { return handle_arith(IROpcode::ADD, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprSub* e) { return handle_arith(IROpcode::SUB, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprMult* e) { return handle_arith(IROpcode::MUL, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprDiv* e) { return handle_arith(IROpcode::DIV, e->lhs, e->rhs); }
        
        IROperand operator()(const NodeBinExprEq* e) { return handle_comp(IROpcode::JE, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprNotEq* e) { return handle_comp(IROpcode::JNE, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprGreater* e) { return handle_comp(IROpcode::JG, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprLess* e) { return handle_comp(IROpcode::JL, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprGreaterEq* e) { return handle_comp(IROpcode::JGE, e->lhs, e->rhs); }
        IROperand operator()(const NodeBinExprLessEq* e) { return handle_comp(IROpcode::JLE, e->lhs, e->rhs); }
    };
    return std::visit(BinVisitor{*this}, bin_expr->bin_expr);
}

IROperand IRBuilder::generate_atom(const NodeAtom* atom) {
    struct AtomVisitor {
        IRBuilder& builder;
        IROperand operator()(const NodeAtomIntLit* l) {
            return IROperand::make_lit(std::stoull(std::string(l->int_lit.value.value())));
        }
        IROperand operator()(const NodeAtomIdent* i) {
            auto* var = builder.find_var(std::string(i->ident.value.value()));
            // If the variable is an array (pointer), this returns the pointer (address).
            // If it's a value, it returns the value vreg.
            // This abstraction works well for 3AC.
            return var->reg;
        }
        IROperand operator()(const NodeAtomParen* p) {
            return builder.generate_expr(p->expr);
        }
        IROperand operator()(const NodeAtomBoolLit* b) {
            return IROperand::make_lit(b->bool_lit.type == TokenType::true_ ? 1 : 0);
        }
        IROperand operator()(const NodeAtomArrayAccess* a) {
            auto* var = builder.find_var(std::string(a->ident.value.value()));
            
            // 1. Calculate Offset: index * 8
            const IROperand idx = builder.generate_expr(a->index);
            const IROperand offset = builder.create_vreg();
            builder.emit(IRInstruction::make_binary(IROpcode::MUL, offset, idx, IROperand::make_lit(8)));
            
            // 2. Calculate Address: base + offset
            const IROperand addr = builder.create_vreg();
            builder.emit(IRInstruction::make_binary(IROpcode::ADD, addr, var->reg, offset));
            
            // 3. Load
            IROperand result = builder.create_vreg();
            builder.emit({IROpcode::LOAD, result, addr, {}});
            
            return result;
        }
    };
    return std::visit(AtomVisitor{*this}, atom->primary_expr);
}

//helpers
IROperand IRBuilder::create_vreg() const
{
    return IROperand::make_reg(m_current_func->vreg_count++);
}

std::string IRBuilder::create_label()
{
    return ".L" + std::to_string(m_label_counter++);
}

void IRBuilder::emit(const IRInstruction &instr) const
{
    m_current_block->instructions.push_back(instr);
}

void IRBuilder::push_scope() {
    m_scopes.emplace_back();
}

void IRBuilder::pop_scope() {
    m_scopes.pop_back();
}

IRBuilder::VarInfo* IRBuilder::find_var(const std::string& name) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

void IRBuilder::add_var(const std::string& name, VarInfo info) {
    m_scopes.back().insert({name, info});
}