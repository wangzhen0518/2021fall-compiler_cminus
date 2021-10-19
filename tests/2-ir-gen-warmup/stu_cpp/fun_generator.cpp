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
    auto module = new Module("fun.c");
    auto builder = new IRBuilder(nullptr, module);

    Type *Int32Type = Type::get_int32_type(module);

    // callee function
    // head part of function
    std::vector<Type *> Int(1, Int32Type);
    auto calleeFunTy = FunctionType::get(Int32Type, Int);
    auto calleeFun = Function::create(calleeFunTy, "callee", module);
    auto bb = BasicBlock::create(module, "entry", calleeFun);
    builder->set_insert_point(bb);

    auto aAlloca = builder->create_alloca(Int32Type); // allocate space for a

    std::vector<Value *> args; // 获取gcd函数的形参,通过Function中的iterator
    for (auto arg = calleeFun->arg_begin(); arg != calleeFun->arg_end(); arg++)
    {
        args.push_back(*arg);
    }

    builder->create_store(args[0], aAlloca); // store a

    auto aLoad = builder->create_load(aAlloca); // load a

    auto mul = builder->create_imul(CONST_INT(2), aLoad); // mul = 2 * a
    builder->create_ret(mul);

    // main fuction
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);

    auto retAlloca = builder->create_alloca(Int32Type); // allocate space for return value
    builder->create_store(CONST_INT(0), retAlloca);

    auto call = builder->create_call(calleeFun, {CONST_INT(110)});

    builder->create_ret(call);

    std::cout << module->print();
    delete module;
    return 0;
}