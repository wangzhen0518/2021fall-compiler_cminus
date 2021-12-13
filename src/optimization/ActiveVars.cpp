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
                        if (use.find(l_val) == use.end())
                            def.insert(l_val);
                        if ((def.find(r_val) == def.end()) &&
                            (dynamic_cast<Constant *>(r_val) == nullptr))
                            use.insert(r_val);
                    } else if (instr->is_br()) {
                        if (dynamic_cast<BranchInst *>(instr)->is_cond_br()) {
                            auto l_val = instr->get_operand(0);
                            if (def.find(l_val) == def.end() &&
                                dynamic_cast<Constant *>(l_val) == nullptr)
                                use.insert(l_val);
                        }
                    } else if (instr->is_phi()) {
                        for (int i = 0; i < instr->get_num_operand() / 2; i++) {
                            auto val = instr->get_operand(2 * i);
                            if (def.find(val) == def.end() &&
                                dynamic_cast<Constant *>(val) == nullptr) {
                                use.insert(val);
                                phi_uses[bb].insert(
                                    {val, dynamic_cast<BasicBlock *>(
                                              instr->get_operand(2 * i + 1))});
                            }
                        }
                        if (use.find(instr) == use.end())
                            def.insert(instr);
                    } else if (instr->is_ret()) {
                        if (!dynamic_cast<ReturnInst *>(instr)->is_void_ret()) {
                            auto l_val = instr->get_operand(0);
                            if (def.find(l_val) == def.end() &&
                                dynamic_cast<Constant *>(l_val) == nullptr)
                                use.insert(l_val);
                        }
                    } else if (instr->is_call()) {
                        for (int i = 1; i < instr->get_num_operand(); i++) {
                            auto val = instr->get_operand(i);
                            if ((def.find(val) == def.end()) &&
                                (dynamic_cast<Constant *>(val) == nullptr))
                                use.insert(val);
                        }
                        if (use.find(instr) == use.end())
                            def.insert(instr);
                    } else {
                        for (int i = 0; i < instr->get_num_operand(); i++) {
                            auto val = instr->get_operand(i);
                            if (def.find(val) == def.end() &&
                                dynamic_cast<Constant *>(val) == nullptr)
                                use.insert(val);
                        }
                        if (use.find(instr) == use.end())
                            def.insert(instr);
                    }
                }
                bb2use[bb] = use;
                bb2def[bb] = def;
            }
            // initialize
            for (auto bb : func->get_basic_blocks()) {
                std::set<Value *> empty_set;
                live_in[bb] = empty_set;
                live_out[bb] = empty_set;
            }
            bool is_change = true;
            while (is_change) {
                is_change = false;
                for (auto bb : func->get_basic_blocks()) {
                    // out = union(succ)
                    for (auto s : bb->get_succ_basic_blocks())
                        for (auto val : live_in[s])
                            if (phi_uses.find(s) == phi_uses.end() ||
                                phi_uses[s].find(val) == phi_uses[s].end() ||
                                phi_uses[s][val] == bb)
                                live_out[bb].insert(val);

                    // in = use union (out sub def)
                    std::set<Value *> new_in_bb = live_out[bb];
                    for (auto op : bb2def[bb])
                        new_in_bb.erase(op);
                    for (auto op : bb2use[bb])
                        new_in_bb.insert(op);
                    if (new_in_bb != live_in[bb])
                        is_change = true;
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
