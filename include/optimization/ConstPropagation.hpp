#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP
#include <stack>
#include <unordered_map>
#include <vector>

#include "Constant.h"
#include "IRBuilder.h"
#include "Instruction.h"
#include "Module.h"
#include "PassManager.hpp"
#include "Value.h"

// tips: 用来判断value是否为ConstantFP/ConstantInt
ConstantFP* cast_constantfp(Value* value);
ConstantInt* cast_constantint(Value* value);

// tips: ConstFloder类
class ConstFolder {
public:
    ConstFolder(Module* m) : module_(m) {}
    Constant* compute(Instruction* instr, ConstantInt* value1,
                         ConstantInt* value2);
    Constant* compute(Instruction* instr, ConstantFP* value1,
                        ConstantFP* value2);
    Constant* compute(Instruction* instr, Constant* value1, Constant* value2);
    // ...
private:
    Module* module_;
};

class ConstPropagation : public Pass {
public:
    ConstPropagation(Module* m) : Pass(m) {}
    void run();
};

#endif
