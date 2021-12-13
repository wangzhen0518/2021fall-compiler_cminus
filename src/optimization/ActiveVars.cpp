#include "ActiveVars.hpp"

#include <map>
#include <unordered_map>

#include "iostream"
#include "set"
#include "vector"

void ActiveVars::run() {
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        } else {
            func_ = func;
            func_->set_instr_name();
            live_in.clear();
            live_out.clear();
            for (auto bb : func_->get_basic_blocks()) {
                std::set<Value *> use;
                std::set<Value *> def;
                for (auto instr : bb->get_instructions()) {
                    if (instr->is_alloca()) {
                        def.insert(instr);
                    } else if (instr->is_store()) {
                        auto l_val =
                            dynamic_cast<StoreInst *>(instr)->get_lval();
                        auto r_val =
                            dynamic_cast<StoreInst *>(instr)->get_rval();
                        if ((def.find(l_val) == def.end()) &&
                            (dynamic_cast<ConstantInt *>(l_val) == nullptr)) {
                            use.insert(l_val);
                        }
                        if ((def.find(r_val) == def.end()) &&
                            (dynamic_cast<ConstantInt *>(r_val) == nullptr) &&
                            (dynamic_cast<ConstantFP *>(r_val) == nullptr)) {
                            use.insert(r_val);
                        }
                    } else if (instr->is_load() || instr->is_zext() ||
                               instr->is_si2fp() || instr->is_fp2si()) {
                        auto l_val = instr->get_operand(0);
                        if (def.find(l_val) == def.end()) {
                            use.insert(l_val);
                        }
                        if (use.find(instr) == use.end()) {
                            def.insert(instr);
                        }
                    } else if (instr->is_cmp() || instr->is_fcmp() ||
                               instr->isBinary()) {
                        auto l_val = instr->get_operand(0);
                        auto r_val = instr->get_operand(1);
                        if (def.find(l_val) == def.end() &&
                            dynamic_cast<ConstantInt *>(l_val) == nullptr &&
                            dynamic_cast<ConstantFP *>(l_val) == nullptr) {
                            use.insert(l_val);
                        }
                        if (def.find(r_val) == def.end() &&
                            dynamic_cast<ConstantInt *>(r_val) == nullptr &&
                            dynamic_cast<ConstantFP *>(r_val) == nullptr) {
                            use.insert(r_val);
                        }
                        if (use.find(instr) == use.end()) {
                            def.insert(instr);
                        }
                    } else if (instr->is_gep()) {
                        for (int i = 0; i < instr->get_num_operand(); i++) {
                            auto val = instr->get_operand(i);
                            if (def.find(val) == def.end() &&
                                dynamic_cast<ConstantInt *>(val) == nullptr &&
                                dynamic_cast<ConstantFP *>(val) == nullptr) {
                                use.insert(val);
                            }
                            if (use.find(instr) == use.end()) {
                                def.insert(instr);
                            }
                        }
                    } else if (instr->is_br()) {
                        auto l_val = instr->get_operand(0);
                        if (def.find(l_val) == def.end() &&
                            dynamic_cast<ConstantInt *>(l_val) == nullptr &&
                            dynamic_cast<ConstantFP *>(l_val) == nullptr &&
                            dynamic_cast<BranchInst *>(instr)->is_cond_br()) {
                            use.insert(l_val);
                        }
                    } else if (instr->is_call()) {
                        for (int i = 1; i < instr->get_num_operand(); i++) {
                            auto val = instr->get_operand(i);
                            if ((def.find(val) == def.end()) &&
                                (dynamic_cast<ConstantInt *>(val) == nullptr) &&
                                (dynamic_cast<ConstantFP *>(val) == nullptr)) {
                                use.insert(val);
                            }
                        }
                        if ((use.find(instr) == use.end()) &&
                            !instr->is_void()) {
                            def.insert(instr);
                        }
                    } else if (instr->is_ret() &&
                               !dynamic_cast<ReturnInst *>(instr)
                                    ->is_void_ret()) {
                        auto l_val = instr->get_operand(0);
                        if (def.find(l_val) == def.end() &&
                            dynamic_cast<ConstantInt *>(l_val) == nullptr &&
                            dynamic_cast<ConstantFP *>(l_val) == nullptr) {
                            use.insert(l_val);
                        }
                    } else if (instr->is_phi()) {
                        for (int i = 0; i < instr->get_num_operand() / 2; i++) {
                            auto val = instr->get_operand(2 * i);
                            if (def.find(val) == def.end() &&
                                dynamic_cast<ConstantInt *>(val) == nullptr &&
                                dynamic_cast<ConstantFP *>(val) == nullptr) {
                                use.insert(val);
                                phi_uses[bb].insert(
                                    {val, dynamic_cast<BasicBlock *>(
                                              instr->get_operand(2 * i + 1))});
                            }
                        }
                        if (use.find(instr) == use.end()) {
                            def.insert(instr);
                        }
                    }
                }
                bb2use[bb] = use;
                bb2def[bb] = def;
            }
            for (auto bb : func->get_basic_blocks()) {
                std::set<Value *> empty_set;
                live_in[bb] = empty_set;
                live_out[bb] = empty_set;
            }
            bool is_change = true;
            while (is_change) {
                is_change = false;
                for (auto bb : func->get_basic_blocks()) {
                    for (auto s : bb->get_succ_basic_blocks()) {
                        for (auto b : live_in[s]) {
                            if (phi_uses.find(s) == phi_uses.end() ||
                                phi_uses[s].find(b) == phi_uses[s].end() ||
                                phi_uses[s][b] == bb) {
                                live_out[bb].insert(b);
                            }
                        }
                    }
                    std::set<Value *> new_in_bb = live_out[bb];
                    for (auto op : bb2def[bb]) {
                        if (new_in_bb.find(op) != new_in_bb.end()) {
                            new_in_bb.erase(op);
                        }
                    }
                    for (auto op : bb2use[bb]) {
                        new_in_bb.insert(op);
                    }
                    if (new_in_bb != live_in[bb]) {
                        is_change = true;
                    }
                    live_in[bb] = new_in_bb;
                }
            }
            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return;
}

std::string ActiveVars::print() {
    std::string active_vars;
    active_vars += "{\n";
    active_vars += "\"function\": \"";
    active_vars += func_->get_name();
    active_vars += "\",\n";

    active_vars += "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars += "  \"";
            active_vars += p.first->get_name();
            active_vars += "\": [";
            for (auto &v : p.second) {
                active_vars += "\"%";
                active_vars += v->get_name();
                active_vars += "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    },\n";

    active_vars += "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars += "  \"";
            active_vars += p.first->get_name();
            active_vars += "\": [";
            for (auto &v : p.second) {
                active_vars += "\"%";
                active_vars += v->get_name();
                active_vars += "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}
