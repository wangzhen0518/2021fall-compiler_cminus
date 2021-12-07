#include "ConstPropagation.hpp"

#include <dbg.h>

#include <unordered_map>
#include <unordered_set>

#include "logging.hpp"

/**
 *! maybe a better choice, parameter is Value type
 *! use dynamic_cast to change type in the fuction
 */

// 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式
// integer
Constant* ConstFolder::compute(Instruction* instr, ConstantInt* value1,
                               ConstantInt* value2) {
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (instr->get_instr_type()) {
        case Instruction::add:
            return ConstantInt::get(c_value1 + c_value2, module_);
        case Instruction::sub:
            return ConstantInt::get(c_value1 - c_value2, module_);
        case Instruction::mul:
            return ConstantInt::get(c_value1 * c_value2, module_);
        case Instruction::sdiv:
            if (c_value2 != 0)
                return ConstantInt::get((int)(c_value1 / c_value2), module_);
        case Instruction::cmp: {
            auto cmp_instr = dynamic_cast<CmpInst*>(instr);
            switch (cmp_instr->get_cmp_op()) {
                case CmpInst::EQ:
                    return ConstantInt::get(c_value1 == c_value2, module_);
                case CmpInst::NE:
                    return ConstantInt::get(c_value1 != c_value2, module_);
                case CmpInst::GT:
                    return ConstantInt::get(c_value1 > c_value2, module_);
                case CmpInst::GE:
                    return ConstantInt::get(c_value1 >= c_value2, module_);
                case CmpInst::LT:
                    return ConstantInt::get(c_value1 < c_value2, module_);
                case CmpInst::LE:
                    return ConstantInt::get(c_value1 <= c_value2, module_);
                default:
                    return nullptr;
            }
        }
        default:
            return nullptr;
    }
}
// both float
Constant* ConstFolder::compute(Instruction* instr, ConstantFP* value1,
                               ConstantFP* value2) {
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (instr->get_instr_type()) {
        case Instruction::fadd:
            return ConstantFP::get(c_value1 + c_value2, module_);
        case Instruction::fsub:
            return ConstantFP::get(c_value1 - c_value2, module_);
        case Instruction::fmul:
            return ConstantFP::get(c_value1 * c_value2, module_);
        case Instruction::fdiv:
            if (c_value2 != 0.0)
                return ConstantFP::get(c_value1 / c_value2, module_);
        case Instruction::fcmp: {
            auto cmp_instr = dynamic_cast<FCmpInst*>(instr);
            switch (cmp_instr->get_cmp_op()) {
                case FCmpInst::EQ:
                    return ConstantInt::get(c_value1 == c_value2, module_);
                case FCmpInst::NE:
                    return ConstantInt::get(c_value1 != c_value2, module_);
                case FCmpInst::GT:
                    return ConstantInt::get(c_value1 > c_value2, module_);
                case FCmpInst::GE:
                    return ConstantInt::get(c_value1 >= c_value2, module_);
                case FCmpInst::LT:
                    return ConstantInt::get(c_value1 < c_value2, module_);
                case FCmpInst::LE:
                    return ConstantInt::get(c_value1 <= c_value2, module_);
                default:
                    return nullptr;
            }
        }
        default:
            return nullptr;
    }
}
Constant* ConstFolder::compute(Instruction* instr, Constant* value1,
                               Constant* value2) {
    auto constant_int_ptr1 = dynamic_cast<ConstantInt*>(value1);
    auto constant_int_ptr2 = dynamic_cast<ConstantInt*>(value2);
    if (constant_int_ptr1 && constant_int_ptr2) {
        // both integer
        dbg("both integer");
        int c_value1 = constant_int_ptr1->get_value();
        int c_value2 = constant_int_ptr2->get_value();
        switch (instr->get_instr_type()) {
            case Instruction::add:
                return ConstantInt::get(c_value1 + c_value2, module_);
            case Instruction::sub:
                return ConstantInt::get(c_value1 - c_value2, module_);
            case Instruction::mul:
                return ConstantInt::get(c_value1 * c_value2, module_);
            case Instruction::sdiv:
                if (c_value2 != 0)
                    return ConstantInt::get((int)(c_value1 / c_value2),
                                            module_);
            case Instruction::cmp: {
                auto cmp_instr = dynamic_cast<CmpInst*>(instr);
                switch (cmp_instr->get_cmp_op()) {
                    case CmpInst::EQ:
                        return ConstantInt::get(c_value1 == c_value2, module_);
                    case CmpInst::NE:
                        return ConstantInt::get(c_value1 != c_value2, module_);
                    case CmpInst::GT:
                        return ConstantInt::get(c_value1 > c_value2, module_);
                    case CmpInst::GE:
                        return ConstantInt::get(c_value1 >= c_value2, module_);
                    case CmpInst::LT:
                        return ConstantInt::get(c_value1 < c_value2, module_);
                    case CmpInst::LE:
                        return ConstantInt::get(c_value1 <= c_value2, module_);
                    default:
                        return nullptr;
                }
            }
            default:
                return nullptr;
        }
    } else {
        // both are float
        dbg("both float");
        auto constant_fp_ptr1 = dynamic_cast<ConstantFP*>(value1);
        auto constant_fp_ptr2 = dynamic_cast<ConstantFP*>(value2);
        float c_value1, c_value2;
        c_value1 = constant_fp_ptr1->get_value();
        c_value2 = constant_fp_ptr2->get_value();

        switch (instr->get_instr_type()) {
            case Instruction::fadd:
                return ConstantFP::get(c_value1 + c_value2, module_);
            case Instruction::fsub:
                return ConstantFP::get(c_value1 - c_value2, module_);
            case Instruction::fmul:
                return ConstantFP::get(c_value1 * c_value2, module_);
            case Instruction::fdiv:
                if (c_value2 != 0)
                    return ConstantFP::get(c_value1 / c_value2, module_);
            case Instruction::fcmp: {
                auto cmp_instr = dynamic_cast<FCmpInst*>(instr);
                switch (cmp_instr->get_cmp_op()) {
                    case FCmpInst::EQ:
                        return ConstantInt::get(c_value1 == c_value2, module_);
                    case FCmpInst::NE:
                        return ConstantInt::get(c_value1 != c_value2, module_);
                    case FCmpInst::GT:
                        return ConstantInt::get(c_value1 > c_value2, module_);
                    case FCmpInst::GE:
                        return ConstantInt::get(c_value1 >= c_value2, module_);
                    case FCmpInst::LT:
                        return ConstantInt::get(c_value1 < c_value2, module_);
                    case FCmpInst::LE:
                        return ConstantInt::get(c_value1 <= c_value2, module_);
                    default:
                        return nullptr;
                }
            }
            default:
                return nullptr;
        }
    }
}

// 用来判断value是否为ConstantFP，如果不是则会返回nullptr
inline ConstantFP* cast_constantfp(Value* value) {
    auto constant_fp_ptr = dynamic_cast<ConstantFP*>(value);
    if (constant_fp_ptr)
        return constant_fp_ptr;
    else
        return nullptr;
}
// 用来判断value是否为ConstantInt，如果不是则会返回nullptr
inline ConstantInt* cast_constantint(Value* value) {
    auto constant_int_ptr = dynamic_cast<ConstantInt*>(value);
    if (constant_int_ptr)
        return constant_int_ptr;
    else
        return nullptr;
}
// 用来判断value是否为Constant，如果不是则会返回nullptr
inline Constant* cast_constant(Value* value) {
    auto constant_ptr = dynamic_cast<Constant*>(value);
    if (constant_ptr)
        return constant_ptr;
    else
        return nullptr;
}
// 用来判断value是否为GlobalVariable，如果不是则会返回nullptr
inline GlobalVariable* cast_globalvar(Value* value) {
    auto global_ptr = dynamic_cast<GlobalVariable*>(value);
    if (global_ptr)
        return global_ptr;
    else
        return nullptr;
}

void ConstPropagation::run() {
    dbg(this);
    dbg("in constant propagation run");
    ConstFolder* floder_ = new ConstFolder(m_);
    for (auto func : this->m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            std::vector<Instruction*> wait_delete;
            /**
             * These three use for global variable.
             * When store a value to a global variable, judge whether it is a
             * constant.
             * If it is, store the glabol variable and the key into
             * global2*val(decided by the global variable's type). Then this
             * value can be used in next load instruction. Remember, replace all
             * the instruction value that uses the result of load instruction.
             * If it is not a constant, remove the global variable from
             * global2*val.
             * ! As it may jump from a block to another block, we can only
             * ! transmit the global variable's value in a same block
             */
            std::unordered_set<GlobalVariable*> global_is_const;
            std::unordered_map<GlobalVariable*, int> global2ival;
            std::unordered_map<GlobalVariable*, float> global2fval;
            for (auto instr : bb->get_instructions()) {
                auto op_ = instr->get_instr_type();
                int op_num = instr->get_num_operand();
                auto& operands = instr->get_operands();
                switch (op_) {
                    case Instruction::ret:
                        break;
                    case Instruction::br:
                        break;
                    case Instruction::add:
                    case Instruction::sub:
                    case Instruction::mul:
                    case Instruction::sdiv:
                    case Instruction::cmp: {
                        ConstantInt *lptr, *rptr;
                        lptr = dynamic_cast<ConstantInt*>(operands[0]);
                        rptr = dynamic_cast<ConstantInt*>(operands[1]);
                        if (lptr && rptr) {
                            auto result = floder_->compute(instr, lptr, rptr);
                            instr->replace_all_use_with(result);
                            wait_delete.emplace_back(instr);
                        }
                    } break;
                    case Instruction::fadd:
                    case Instruction::fsub:
                    case Instruction::fmul:
                    case Instruction::fdiv:
                    case Instruction::fcmp: {
                        ConstantFP *lptr, *rptr;
                        lptr = dynamic_cast<ConstantFP*>(operands[0]);
                        rptr = dynamic_cast<ConstantFP*>(operands[1]);
                        if (lptr && rptr) {
                            auto result = floder_->compute(instr, lptr, rptr);
                            instr->replace_all_use_with(result);
                            wait_delete.emplace_back(instr);
                        }
                    } break;
                    case Instruction::alloca:
                        break;
                    case Instruction::load: {
                        GlobalVariable* global_var =
                            dynamic_cast<GlobalVariable*>(operands[0]);
                        if (global_var && global_is_const.find(global_var) !=
                                              global_is_const.cend()) {
                            if (global_var->get_type()
                                    ->get_pointer_element_type()
                                    ->is_integer_type()) {
                                instr->replace_all_use_with(ConstantInt::get(
                                    global2ival[global_var], m_));
                                wait_delete.emplace_back(instr);
                            } else if (global_var->get_type()
                                           ->get_pointer_element_type()
                                           ->is_float_type()) {
                                instr->replace_all_use_with(ConstantFP::get(
                                    global2fval[global_var], m_));
                                wait_delete.emplace_back(instr);
                            }
                        }
                    } break;
                    case Instruction::store: {
                        GlobalVariable* global_var =
                            dynamic_cast<GlobalVariable*>(operands[1]);
                        if (global_var) {
                            if (global_var->get_type()
                                    ->get_pointer_element_type()
                                    ->is_integer_type()) {
                                ConstantInt* intptr =
                                    dynamic_cast<ConstantInt*>(operands[0]);
                                if (intptr) {
                                    int val = intptr->get_value();
                                    global2ival[global_var] = val;
                                    global_is_const.insert(global_var);
                                } else {
                                    global2ival.erase(global_var);
                                    global_is_const.erase(global_var);
                                }
                            } else if (global_var->get_type()
                                           ->get_pointer_element_type()
                                           ->is_float_type()) {
                                ConstantFP* fptr =
                                    dynamic_cast<ConstantFP*>(operands[0]);
                                if (fptr) {
                                    float val = fptr->get_value();
                                    global2fval[global_var] = val;
                                    global_is_const.insert(global_var);
                                } else {
                                    global2fval.erase(global_var);
                                    global_is_const.erase(global_var);
                                }
                            }
                        }
                    } break;
                    case Instruction::phi:
                        break;
                    case Instruction::call:
                        break;
                    case Instruction::getelementptr:
                        break;
                    case Instruction::zext: {
                        ConstantInt* intptr =
                            dynamic_cast<ConstantInt*>(operands[0]);
                        if (intptr) {
                            instr->replace_all_use_with(
                                ConstantInt::get(intptr->get_value(), m_));
                            wait_delete.emplace_back(instr);
                        }
                    } break;
                    case Instruction::fptosi: {
                        ConstantFP* fptr =
                            dynamic_cast<ConstantFP*>(operands[0]);
                        if (fptr) {
                            instr->replace_all_use_with(
                                ConstantInt::get(int(fptr->get_value()), m_));
                            wait_delete.emplace_back(instr);
                        }
                    } break;
                    case Instruction::sitofp: {
                        ConstantInt* intptr =
                            dynamic_cast<ConstantInt*>(operands[0]);
                        if (intptr) {
                            instr->replace_all_use_with(
                                ConstantFP::get(intptr->get_value(), m_));
                            wait_delete.emplace_back(instr);
                        }
                    } break;
                    default:
                        break;
                }  // end switch
                dbg(instr);
            }  // end for (auto instr : bb->get_instructions())
            for (auto instr : wait_delete)
                bb->delete_instr(instr);
            dbg(bb);
        }  // end for (auto bb : func->get_basic_blocks())
        dbg(func);
    }  // for (auto func : this->m_->get_functions())
    delete floder_;
    dbg("here");
}
