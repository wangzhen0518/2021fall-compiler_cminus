#include "cminusf_builder.hpp"

#define DEBUG
#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl  // 输出行号的简单示例
#define DEBUG_INFO(S) std::cout << S << std::endl
#else
#define DEBUG_OUTPUT
#endif

#define ERROR(comment) std::cout << comment

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get((int)num, module.get())
#define CONST_ZERO(type) ConstantZero::get(type, module.get())

#define int32_type Type::get_int32_type(module.get())
#define float_type Type::get_float_type(module.get())
#define void_type Type::get_void_type(module.get())
#define int32_ptr_type Type::get_int32_ptr_type(module.get())
#define float_ptr_type Type::get_float_ptr_type(module.get())

#define MAX_INDEX 10'000'000

int index_count;
int ast_num_i;
float ast_num_f;
CminusType ast_num_type;
// Constant* ast_num_val;
Function* current_func;
AllocaInst* return_val;
Value* ast_val;  // value of function call, expression, et

void judge_type(Type* t) {
    if (t->is_array_type())
        std::cout << "array type\n";
    else if (t->is_function_type())
        std::cout << "function type\n";
    else if (t->is_float_type())
        std::cout << "float type\n";
    else if (t->is_integer_type())
        std::cout << "integer type\n";
    else if (t->is_pointer_type())
        std::cout << "pointer type\n";
    else if (t->is_void_type())
        std::cout << "void type\n";
    else if (t->is_label_type())
        std::cout << "label type\n";
}

void CminusfBuilder::visit(ASTProgram& node) {
    for (auto& decl : node.declarations)
        decl->accept(*this);
}

void CminusfBuilder::visit(ASTNum& node) {
    DEBUG_INFO("visit num");
    if (node.type == TYPE_INT) {
        ast_num_type = TYPE_INT;
        ast_num_i = node.i_val;
        ast_val = CONST_INT(node.i_val);
    } else if (node.type == TYPE_FLOAT) {
        ast_num_type = TYPE_FLOAT;
        ast_num_f = node.f_val;
        ast_val = CONST_FP(node.f_val);
    }
    DEBUG_INFO("visit num over");
}

void CminusfBuilder::visit(ASTVarDeclaration& node) {
    DEBUG_INFO("visit variable declaration");
    if (scope.in_global()) {
        if (node.num == nullptr) {  // not array
            if (node.type == TYPE_INT) {
                Value* val =
                    GlobalVariable::create(node.id, module.get(), int32_type,
                                           false, CONST_ZERO(int32_type));
                scope.push(node.id, val);
            } else if (node.type == TYPE_FLOAT) {
                Value* val =
                    GlobalVariable::create(node.id, module.get(), float_type,
                                           false, CONST_ZERO(float_type));
                scope.push(node.id, val);
            } else
                ERROR("Cannot define a variable of void type\n");
        } else {  // array
            if (node.type == TYPE_INT) {
                node.num->accept(*this);
                if (ast_num_i <= 0)
                    ERROR("number of an array members cannot be less than 1\n");
                else {
                    auto* arrayType = ArrayType::get(int32_type, ast_num_i);
                    Value* val =
                        GlobalVariable::create(node.id, module.get(), arrayType,
                                               false, CONST_ZERO(arrayType));
                    scope.push(node.id, val);
                }
            } else if (node.type == TYPE_FLOAT) {
                node.num->accept(*this);
                if (ast_num_i <= 0)
                    ERROR("number of an array members cannot be less than 1\n");
                else {
                    auto* arrayType = ArrayType::get(float_type, ast_num_i);
                    Value* val =
                        GlobalVariable::create(node.id, module.get(), arrayType,
                                               false, CONST_ZERO(arrayType));
                    scope.push(node.id, val);
                }
            } else
                ERROR("Cannot define a variable of void type\n");
        }
    } else {
        if (node.num == nullptr) {
            if (node.type == TYPE_INT) {
                Value* val = builder->create_alloca(int32_type);
                scope.push(node.id, val);
            } else if (node.type == TYPE_FLOAT) {
                Value* val = builder->create_alloca(float_type);
                scope.push(node.id, val);
            } else
                ERROR("Cannot define a variable of void type\n");
        } else {
            if (node.type == TYPE_INT) {
                node.num->accept(*this);
                if (ast_num_i <= 0)
                    ERROR("number of an array members cannot be less than 1\n");
                else {
                    auto* arrayType = ArrayType::get(int32_type, ast_num_i);
                    Value* val = builder->create_alloca(arrayType);
                    scope.push(node.id, val);
                }
            } else if (node.type == TYPE_FLOAT) {
                node.num->accept(*this);
                if (ast_num_i <= 0)
                    ERROR("number of an array members cannot be less than 1\n");
                else {
                    auto* arrayType = ArrayType::get(float_type, ast_num_i);
                    Value* val = builder->create_alloca(arrayType);
                    scope.push(node.id, val);
                }
            } else
                ERROR("Cannot define a variable of void type\n");
        }
    }
    DEBUG_INFO("visit variable declaration over");
}

void CminusfBuilder::visit(ASTFunDeclaration& node) {
    DEBUG_INFO("visit function declaration");
    // parameter list
    std::vector<Type*> param_types;
    if (node.params.size() != 0 && node.params[0]->type != TYPE_VOID) {
        for (auto param : node.params) {
            if (param->type == TYPE_VOID)
                ERROR("A parameter in parameter list cannot be a void type\n");
            else if (param->type == TYPE_INT) {
                if (param->isarray)
                    param_types.push_back(int32_ptr_type);
                else
                    param_types.push_back(int32_type);
            } else {  // TYPE_FLOAT
                if (param->isarray)
                    param_types.push_back(float_ptr_type);
                else
                    param_types.push_back(float_type);
            }
        }
    }

    // return type
    Type* return_type;
    if (node.type == TYPE_VOID)
        return_type = void_type;
    else if (node.type == TYPE_INT)
        return_type = int32_type;
    else  // TYPE_FLOAT
        return_type = float_type;

    auto func = Function::create(FunctionType::get(return_type, param_types),
                                 node.id, module.get());
    current_func = func;

    auto bb = BasicBlock::create(module.get(), node.id, func);
    builder->set_insert_point(bb);
    scope.push(node.id, func);
    scope.enter();

    if (node.type == TYPE_INT)
        return_val = builder->create_alloca(int32_type);
    else if (node.type == TYPE_FLOAT)
        return_val = builder->create_alloca(float_type);

    if (node.params.size() != 0 && node.params[0]->type != TYPE_VOID) {
        for (auto param : node.params) {
            if (param->type == TYPE_VOID)
                ERROR("A parameter in parameter list cannot be void type\n");
            else
                param->accept(*this);
        }
    }

    std::vector<Value*> args;
    for (auto arg = func->arg_begin(); arg != func->arg_end(); arg++)
        args.push_back(*arg);
    for (int i = 0; i < node.params.size(); i++)
        builder->create_store(args[i], scope.find(node.params[i]->id));
    node.compound_stmt->accept(*this);
    scope.exit();
    DEBUG_INFO("visit function declaration over");
}

void CminusfBuilder::visit(ASTParam& node) {
    Value* param_ptr;
    if (node.isarray) {
        if (node.type == TYPE_INT)
            param_ptr = builder->create_alloca(int32_ptr_type);
        else
            param_ptr = builder->create_alloca(float_ptr_type);

    } else {
        if (node.type == TYPE_INT)
            param_ptr = builder->create_alloca(int32_type);
        else
            param_ptr = builder->create_alloca(float_type);
    }
    scope.push(node.id, param_ptr);
}

void CminusfBuilder::visit(ASTCompoundStmt& node) {
    DEBUG_INFO("visit compound statement");
    for (auto& decl : node.local_declarations)
        decl->accept(*this);
    for (auto& stmt : node.statement_list)
        stmt->accept(*this);
    DEBUG_INFO("visit compound statement over");
}

void CminusfBuilder::visit(ASTExpressionStmt& node) {
    if (node.expression != nullptr)
        node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt& node) {}

void CminusfBuilder::visit(ASTIterationStmt& node) {}

void CminusfBuilder::visit(ASTReturnStmt& node) {}

void CminusfBuilder::visit(ASTVar& node) {
    DEBUG_INFO("visit variable");
    auto var = scope.find(node.id);
    // if var is 'a[i]', it is necessary to get 'a[i]'
    // as var is 'a' now
    if (node.expression != nullptr) {
        node.expression->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            ast_val = builder->create_load(ast_val);
        if (ast_val->get_type()->is_float_type())
            ast_val = builder->create_fptosi(ast_val, int32_type);
        auto icmp = builder->create_icmp_le(ast_val, CONST_INT(0));
        // check the index
        std::string label_name = node.id + std::to_string(index_count);
        index_count = (index_count + 1) % MAX_INDEX;
        auto trueBB = BasicBlock::create(module.get(), label_name + "_true",
                                         current_func);
        auto falseBB = BasicBlock::create(module.get(), label_name + "_false",
                                          current_func);
        auto br = builder->create_cond_br(icmp, trueBB, falseBB);
        // true basic block
        // index is less than 0, exit
        builder->set_insert_point(trueBB);
        builder->create_call(Function::create(FunctionType::get(void_type, {}),
                                              "neg_idx_except", module.get()),
                             {});
        builder->create_br(falseBB);
        // false basic block
        builder->set_insert_point(falseBB);
        var = builder->create_gep(var, {CONST_INT(0), ast_val});
    }
    // else var is 'a', it is enough
    ast_val = var;
    DEBUG_INFO("visit variable over");
    DEBUG_INFO(node.id);
}

void CminusfBuilder::visit(ASTAssignExpression& node) {
    DEBUG_INFO("visit assignment expression");
    if (node.var == nullptr)
        node.expression->accept(*this);
    else {
        node.var->accept(*this);
        auto l_var = ast_val;
        node.expression->accept(*this);
        DEBUG_OUTPUT;
        Value* r_var;
        if (ast_val->get_type()->is_pointer_type())
            r_var = builder->create_load(ast_val);
        else
            r_var = ast_val;
        DEBUG_OUTPUT;
        auto l_type = l_var->get_type()->get_pointer_element_type();
        DEBUG_OUTPUT;
        auto r_type = r_var->get_type();

        if ((l_type->is_integer_type() && r_type->is_integer_type()) ||
            (l_type->is_float_type() && r_type->is_float_type()))
            // left and right are the same type
            builder->create_store(r_var, l_var);
        else if (l_type->is_integer_type() && r_type->is_float_type()) {
            // left is integer, and right is float
            auto fp_si = builder->create_fptosi(r_var, int32_type);
            builder->create_store(fp_si, l_var);
        } else {  // left is float, and right is integer
            auto si_fp = builder->create_sitofp(r_var, float_type);
            builder->create_store(si_fp, l_var);
        }
    }
    DEBUG_INFO("visit assignment expression over");
}

void CminusfBuilder::visit(ASTSimpleExpression& node) {
    DEBUG_INFO("visit simple expression");
    if (node.additive_expression_r == nullptr)
        node.additive_expression_l->accept(*this);
    else {
        Value *l_val, *r_val;
        node.additive_expression_l->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            l_val = builder->create_load(ast_val);
        else
            l_val = ast_val;
        node.additive_expression_r->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            r_val = builder->create_load(ast_val);
        else
            r_val = ast_val;
        auto l_type = l_val->get_type();
        auto r_type = r_val->get_type();
        switch (node.op) {
            case OP_LE:
                if (l_type->is_integer_type() && r_type->is_integer_type())
                    ast_val = builder->create_icmp_le(l_val, r_val);
                else if (l_type->is_integer_type() && r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fcmp_le(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fcmp_le(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fcmp_le(l_val, r_val);
                break;
            case OP_LT:
                if (l_type->is_integer_type() && r_type->is_integer_type())
                    ast_val = builder->create_icmp_lt(l_val, r_val);
                else if (l_type->is_integer_type() && r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fcmp_lt(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fcmp_lt(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fcmp_lt(l_val, r_val);
                break;
            case OP_GT:
                if (l_type->is_integer_type() && r_type->is_integer_type())
                    ast_val = builder->create_icmp_gt(l_val, r_val);
                else if (l_type->is_integer_type() && r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fcmp_gt(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fcmp_gt(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fcmp_gt(l_val, r_val);
                break;
            case OP_GE:
                if (l_type->is_integer_type() && r_type->is_integer_type())
                    ast_val = builder->create_icmp_ge(l_val, r_val);
                else if (l_type->is_integer_type() && r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fcmp_ge(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fcmp_ge(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fcmp_ge(l_val, r_val);
                break;
            case OP_EQ:
                if (l_type->is_integer_type() && r_type->is_integer_type())
                    ast_val = builder->create_icmp_eq(l_val, r_val);
                else if (l_type->is_integer_type() && r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fcmp_eq(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fcmp_eq(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fcmp_eq(l_val, r_val);
                break;
            case OP_NEQ:
                if (l_type->is_integer_type() && r_type->is_integer_type())
                    ast_val = builder->create_icmp_ne(l_val, r_val);
                else if (l_type->is_integer_type() && r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fcmp_ne(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fcmp_ne(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fcmp_ne(l_val, r_val);
                break;
        }
    }
    DEBUG_INFO("visit simple expression over");
}

void CminusfBuilder::visit(ASTAdditiveExpression& node) {
    DEBUG_INFO("visit additive expression");
    if (node.additive_expression == nullptr)
        node.term->accept(*this);
    else {
        Value *l_val, *r_val;
        node.additive_expression->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            l_val = builder->create_load(ast_val);
        else
            l_val = ast_val;
        node.term->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            r_val = builder->create_load(ast_val);
        else
            r_val = ast_val;
        auto l_type = l_val->get_type();
        auto r_type = r_val->get_type();
        switch (node.op) {
            case OP_PLUS:
                if (l_type->is_integer_type() && r_type->is_integer_type())
                    ast_val = builder->create_iadd(l_val, r_val);
                else if (l_type->is_integer_type() && r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fadd(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fadd(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fadd(l_val, r_val);
                break;
            case OP_MINUS:
                if (l_type->is_integer_type() && r_type->is_integer_type()) {
                    ast_val = builder->create_isub(l_val, r_val);
                } else if (l_type->is_integer_type() &&
                           r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fsub(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fsub(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fsub(l_val, r_val);
                break;
        }
    }
    DEBUG_INFO("visit additive expression over");
}

void CminusfBuilder::visit(ASTTerm& node) {
    DEBUG_INFO("visit term");
    if (node.term == nullptr)
        node.factor->accept(*this);
    else {
        Value *l_val, *r_val;
        node.term->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            l_val = builder->create_load(ast_val);
        else
            l_val = ast_val;
        node.factor->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            r_val = builder->create_load(ast_val);
        else
            r_val = ast_val;
        auto l_type = l_val->get_type();
        auto r_type = r_val->get_type();
        switch (node.op) {
            case OP_MUL:
                if (l_type->is_integer_type() && r_type->is_integer_type()) {
                    ast_val = builder->create_imul(l_val, r_val);
                } else if (l_type->is_integer_type() &&
                           r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fmul(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fmul(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fmul(l_val, r_val);
                break;
            case OP_DIV:
                if (l_type->is_integer_type() && r_type->is_integer_type()) {
                    ast_val = builder->create_isdiv(l_val, r_val);
                } else if (l_type->is_integer_type() &&
                           r_type->is_float_type()) {
                    auto si_fp = builder->create_sitofp(l_val, l_type);
                    ast_val = builder->create_fdiv(si_fp, r_val);
                } else if (l_type->is_float_type() &&
                           r_type->is_integer_type()) {
                    auto si_fp = builder->create_sitofp(r_val, r_type);
                    ast_val = builder->create_fdiv(l_val, si_fp);
                } else  // both are float_type
                    ast_val = builder->create_fdiv(l_val, r_val);
                break;
        }
    }
    DEBUG_INFO("visit term over");
}

void CminusfBuilder::visit(ASTCall& node) {
    DEBUG_INFO("visit function call");
    Function* func = dynamic_cast<Function*>(scope.find(node.id));
    if (typeid(func) == typeid(Function*))
        std::cout << "convert succeed\n";
    else if (typeid(func) == typeid(Value*))
        std::cout << "convert fail\n";
    if (func == nullptr) {
        ERROR("it is not a function\n");
        builder->create_call(Function::create(FunctionType::get(void_type, {}),
                                              "neg_idx_except", module.get()),
                             {});
        return;
    }
    auto argu_list = func->get_args();
    if (argu_list.size() > node.args.size())
        ERROR("too few arguments in function call\n");
    else if (argu_list.size() < node.args.size())
        ERROR("too many arguments in function call\n");
    else {
        std::vector<Value*> parameters;
        auto arg = argu_list.begin();
        for (auto& param : node.args) {
            param->accept(*this);
            if (ast_val->get_type()->is_pointer_type())
                ast_val = builder->create_load(ast_val);
            // type conversion
            auto param_type = ast_val->get_type();
            auto arg_type = (*arg)->get_type();
            // if arg and param are the same type
            // do not need to do anything
            // if arg is integer, and param is float
            if (arg_type->is_integer_type() && param_type->is_float_type())
                ast_val = builder->create_fptosi(ast_val, int32_type);
            else if (arg_type->is_float_type() && param_type->is_integer_type())
                ast_val = builder->create_sitofp(ast_val, float_type);

            parameters.push_back(ast_val);
            arg++;
        }
        ast_val = builder->create_call(func, parameters);
    }
    DEBUG_INFO("visit function call over");
}
