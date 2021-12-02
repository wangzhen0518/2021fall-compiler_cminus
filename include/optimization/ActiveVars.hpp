#ifndef ACTIVEVARS_HPP
#define ACTIVEVARS_HPP
#include <fstream>
#include <map>
#include <queue>
#include <stack>
#include <unordered_map>
#include <vector>

#include "Constant.h"
#include "IRBuilder.h"
#include "Instruction.h"
#include "Module.h"
#include "PassManager.hpp"
#include "Value.h"

class ActiveVars : public Pass {
public:
    ActiveVars(Module *m) : Pass(m) {}
    void run();
    std::string print();
    std::string use_def_print();

private:
    Function *func_;
    std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
    std::map<BasicBlock *, std::set<Value *>> bb2use, bb2def;
    std::map<BasicBlock *, std::map<Value *, BasicBlock *>> phi_uses;
};

#endif
