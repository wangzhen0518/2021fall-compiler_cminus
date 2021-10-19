#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG                                             // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl; // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main()
{
    auto module = new Module("if.c"); // module name是什么无关紧要
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    Type *FloatType = Type::get_float_type(module);

    // main function
    auto mainFunc = Function::create(FunctionType::get(Int32Type, {}), ",main", module);
    auto bb = BasicBlock::create(module, "entry", mainFunc);
    builder->set_insert_point(bb);

    auto retAlloca = builder->create_alloca(Int32Type); // allocate space for return value
    auto aAlloca = builder->create_alloca(FloatType);   // allocate space for float a
    builder->create_store(CONST_INT(0), retAlloca);
    builder->create_store(CONST_FP(5.555), aAlloca);
    auto aLoad = builder->create_load(aAlloca); // load a
    auto icmp = builder->create_fcmp_gt(aLoad, CONST_FP(1.0));

    auto trueBranch = BasicBlock::create(module, "trueBranch", mainFunc);
    auto falseBranch = BasicBlock::create(module, "falseBranch", mainFunc);
    auto retBranch = BasicBlock::create(module, "retBranch", mainFunc);

    builder->create_cond_br(icmp, trueBranch, falseBranch);

    // true branch
    builder->set_insert_point(trueBranch);
    builder->create_store(CONST_INT(233), retAlloca);
    builder->create_br(retBranch);

    // false branch
    builder->set_insert_point(falseBranch);
    builder->create_store(CONST_INT(0), retAlloca);
    builder->create_br(retBranch);

    // return branch
    builder->set_insert_point(retBranch);
    auto retLoad = builder->create_load(retAlloca); // load return value
    builder->create_ret(retLoad);

    std::cout << module->print();
    delete module;
    return 0;
}