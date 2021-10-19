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
    auto module = new Module("assign.c");
    auto builder = new IRBuilder(nullptr, module);

    Type *Int32Type = Type::get_int32_type(module);
    Type *Int32ArrayType_10 = Type::get_array_type(Int32Type, 10);

    // main fuction
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);

    auto retAlloca = builder->create_alloca(Int32Type);       // allocate return value space
    auto aAlloca = builder->create_alloca(Int32ArrayType_10); // allocate space for array a[10]
    builder->create_store(CONST_INT(0), retAlloca);           // return 0 as default

    auto a0Gep = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(0)}); // get a[0] address
    builder->create_store(CONST_INT(10), a0Gep);                             // store 10 in a[0]

    auto a1Gep = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(1)}); // get a[1] address
    auto a0Load = builder->create_load(a1Gep);                               // get a[0] value
    auto mul = builder->create_imul(a0Load, CONST_INT(2));                   // a[1] = a[0] * 2
    builder->create_store(mul, a1Gep);                                       // store result into a[1]

    auto a1Load = builder->create_load(a1Gep); // get a[1] value
    builder->create_ret(a1Load);

    std::cout << module->print();
    delete module;
    return 0;
}