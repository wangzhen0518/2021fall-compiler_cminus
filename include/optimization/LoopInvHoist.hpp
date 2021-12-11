#pragma once

#include <unordered_map>
#include <unordered_set>
#include "PassManager.hpp"
#include "Module.h"
#include "Function.h"
#include "BasicBlock.h"
#include "LoopSearch.hpp"

class LoopInvHoist : public Pass {
    std::set<std::pair<BasicBlock*,Instruction*>> loop_invariant;
    void find_loop_invariant(BBset_t* loop);
    void moveout_loop_invariant(BBset_t* loop,LoopSearch loop_searcher);
public:
    LoopInvHoist(Module *m) : Pass(m) {}

    void run() override;
};
