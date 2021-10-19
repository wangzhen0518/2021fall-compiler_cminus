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

#define CONST_INT(num) ConstantInt::get(num, module)
#define CONST_FP(num) ConstantFP::get(num, module)

int main()
{
    auto module = new Module("while.c");
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);

    auto mainFunc = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFunc);
    builder->set_insert_point(bb);

    auto retAlloca = builder->create_alloca(Int32Type); // allocate space for return value
    auto aAlloca = builder->create_alloca(Int32Type);   // allocate space for a
    auto iAlloca = builder->create_alloca(Int32Type);   // allocate space for i
    builder->create_store(CONST_INT(0), retAlloca);     // return value = 0
    builder->create_store(CONST_INT(10), aAlloca);      // a = 10
    builder->create_store(CONST_INT(0), iAlloca);       // i = 0

    auto judgeBlock = BasicBlock::create(module, "judgeBlock", mainFunc);
    auto whileBlock = BasicBlock::create(module, "whileBlock", mainFunc);
    auto retBlock = BasicBlock::create(module, "retBlock", mainFunc);

    builder->create_br(judgeBlock);

    // judge block
    builder->set_insert_point(judgeBlock);
    auto iLoad = builder->create_load(iAlloca); // load i value
    auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));
    builder->create_cond_br(icmp, whileBlock, retBlock);

    // while block
    builder->set_insert_point(whileBlock);
    auto iLoad_2 = builder->create_load(iAlloca); // load i value
    auto add = builder->create_iadd(iLoad_2, CONST_INT(1));
    builder->create_store(iLoad_2, iAlloca);           // i = i + 1
    auto aLoad = builder->create_load(aAlloca);        // load a value
    auto add_2 = builder->create_iadd(aLoad, iLoad_2); // a = a + i
    builder->create_store(add_2, aAlloca);
    builder->create_br(judgeBlock);

    // return block
    builder->set_insert_point(retBlock);
    auto aLoad_2 = builder->create_load(aAlloca); // load a value
    builder->create_ret(aLoad_2);

    std::cout << module->print();
    delete module;
    return 0;
}