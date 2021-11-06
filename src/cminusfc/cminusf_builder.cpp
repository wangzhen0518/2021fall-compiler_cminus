#include "cminusf_builder.hpp"

// #define DEBUG
#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl  // 输出行号的简单示例
#define DEBUG_INFO(S) std::cout << S << std::endl
#else
#define DEBUG_OUTPUT
#define DEBUG_INFO(S)
#endif

#define ERROR(comment) std::cout << comment << std::endl

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get((int)num, module.get())
#define CONST_ZERO(type) ConstantZero::get(type, module.get())

#define int1_type Type::get_int1_type(module.get())
#define int32_type Type::get_int32_type(module.get())
#define float_type Type::get_float_type(module.get())
#define void_type Type::get_void_type(module.get())
#define int32_ptr_type Type::get_int32_ptr_type(module.get())
#define float_ptr_type Type::get_float_ptr_type(module.get())

#define MAX_COUNT 10'000'000

int count;
int ast_num_i;
float ast_num_f;
CminusType ast_num_type;
// Constant* ast_num_val;
Function* current_func;
bool is_return = false;
AllocaInst* return_val;
Type* return_type;
BasicBlock* return_block;
Value* ast_val;  // value of function call, expression, et

void judge_type(Type* t, std::unique_ptr<Module>& module) {
    if (t->is_array_type())
        std::cout << "array type\n";
    else if (t->is_function_type())
        std::cout << "function type\n";
    else if (t->is_float_type())
        std::cout << "float type\n";
    else if (t == int32_type)
        std::cout << "int32 type\n";
    else if (t == int1_type)
        std::cout << "int1 type\n";
    else if (t->is_pointer_type())
        std::cout << "pointer type\n";
    else if (t->is_void_type())
        std::cout << "void type\n";
    else if (t->is_label_type())
        std::cout << "label type\n";
}

void type_convert(Value*& v, Type* target, std::unique_ptr<Module>& module,
                  IRBuilder*& builder) {
    auto type_ = v->get_type();
    if (target == int32_type) {
        if (type_ == float_type)
            v = builder->create_fptosi(v, target);
        else if (type_ == int1_type)
            v = builder->create_zext(v, target);
    }
    if (target == float_type) {
        if (type_ == int32_type)
            v = builder->create_sitofp(v, target);
        else if (type_ == int1_type) {
            v = builder->create_zext(v, int32_type);
            v = builder->create_sitofp(v, target);
        }
    }
}
void type_convert(Value*& l, Value*& r, Type* target,
                  std::unique_ptr<Module>& module, IRBuilder*& builder) {
    auto l_type = l->get_type();
    auto r_type = r->get_type();
    if (target == int32_type) {
        if (l_type == float_type)
            l = builder->create_fptosi(l, target);
        else if (l_type == int1_type)
            l = builder->create_zext(l, target);
        if (r_type == float_type)
            r = builder->create_fptosi(r, target);
        else if (r_type == int1_type)
            r = builder->create_zext(r, target);
    }
    if (target == float_type) {
        if (l_type == int32_type)
            l = builder->create_sitofp(l, target);
        else if (l_type == int1_type) {
            l = builder->create_zext(l, int32_type);
            l = builder->create_sitofp(l, target);
        }
        if (r_type == int32_type)
            r = builder->create_sitofp(r, target);
        else if (r_type == int1_type) {
            r = builder->create_zext(r, int32_type);
            r = builder->create_sitofp(r, target);
        }
    }
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
        DEBUG_INFO(ast_num_i);
    } else if (node.type == TYPE_FLOAT) {
        ast_num_type = TYPE_FLOAT;
        ast_num_f = node.f_val;
        ast_val = CONST_FP(node.f_val);
        DEBUG_INFO(ast_num_f);
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
                ERROR("Cannot define a variable of void type");
        } else {  // array
            if (node.type == TYPE_INT) {
                node.num->accept(*this);
                if (ast_num_i <= 0)
                    ERROR("number of an array members cannot be less than 1");
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
                    ERROR("number of an array members cannot be less than 1");
                else {
                    auto* arrayType = ArrayType::get(float_type, ast_num_i);
                    Value* val =
                        GlobalVariable::create(node.id, module.get(), arrayType,
                                               false, CONST_ZERO(arrayType));
                    scope.push(node.id, val);
                }
            } else
                ERROR("Cannot define a variable of void type");
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
                ERROR("Cannot define a variable of void type");
        } else {
            if (node.type == TYPE_INT) {
                node.num->accept(*this);
                if (ast_num_i <= 0)
                    ERROR("number of an array members cannot be less than 1");
                else {
                    auto* arrayType = ArrayType::get(int32_type, ast_num_i);
                    Value* val = builder->create_alloca(arrayType);
                    scope.push(node.id, val);
                }
            } else if (node.type == TYPE_FLOAT) {
                node.num->accept(*this);
                if (ast_num_i <= 0)
                    ERROR("number of an array members cannot be less than 1");
                else {
                    auto* arrayType = ArrayType::get(float_type, ast_num_i);
                    Value* val = builder->create_alloca(arrayType);
                    scope.push(node.id, val);
                }
            } else
                ERROR("Cannot define a variable of void type");
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
                ERROR("A parameter in parameter list cannot be a void type");
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
    return_block = BasicBlock::create(module.get(), "return_" + node.id, func);
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
                ERROR("A parameter in parameter list cannot be void type");
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
    if (return_val != nullptr && return_type->is_void_type())
        builder->create_br(return_block);
    builder->set_insert_point(return_block);
    if (return_val == nullptr || return_type->is_void_type())
        builder->create_void_ret();
    else {
        ast_val = builder->create_load(return_val);
        builder->create_ret(ast_val);
    }
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
    scope.enter();
    for (auto& decl : node.local_declarations)
        decl->accept(*this);
    for (auto& stmt : node.statement_list)
        stmt->accept(*this);
    scope.exit();
    DEBUG_INFO("visit compound statement over");
}

void CminusfBuilder::visit(ASTExpressionStmt& node) {
    DEBUG_INFO("visit expression statement");
    if (node.expression != nullptr)
        node.expression->accept(*this);
    DEBUG_INFO("visit expression statement over");
}

void CminusfBuilder::visit(ASTSelectionStmt& node) {
    // DEBUG_INFO("visit selection statement");
    int current_count = count;
    count = (count + 1) % MAX_COUNT;
    DEBUG_INFO("if" + std::to_string(current_count));
    if (node.expression != nullptr) {
        node.expression->accept(*this);
        if (node.else_statement != nullptr) {  // if ... else ...
            auto truebb = BasicBlock::create(
                module.get(), "true_" + std::to_string(current_count),
                current_func);
            auto falsebb = BasicBlock::create(
                module.get(), "false_" + std::to_string(current_count),
                current_func);
            BasicBlock* exitbb = nullptr;
            if (ast_val->get_type()->is_integer_type()) {
                if (ast_val->get_type() == int1_type)
                    ast_val = builder->create_zext(ast_val, int32_type);
                ast_val = builder->create_icmp_ne(ast_val, {CONST_INT(0)});
            } else
                ast_val = builder->create_fcmp_ne(ast_val, {CONST_FP(0.0)});
            builder->create_cond_br(ast_val, truebb, falsebb);
            // true
            builder->set_insert_point(truebb);
            is_return = false;
            node.if_statement->accept(*this);
            if (!is_return) {
                DEBUG_INFO("true block no return");
                exitbb = BasicBlock::create(
                    module.get(), "exit_" + std::to_string(current_count),
                    current_func);
                builder->create_br(exitbb);
            }
            // false
            builder->set_insert_point(falsebb);
            is_return = false;
            node.else_statement->accept(*this);
            if (!is_return) {
                DEBUG_INFO("false block no return");
                if (exitbb == nullptr)
                    exitbb = BasicBlock::create(
                        module.get(), "exit_" + std::to_string(current_count),
                        current_func);
                builder->create_br(exitbb);
            }
            // exit
            if (!is_return)
                builder->set_insert_point(exitbb);
        } else {  // if ...
            auto truebb = BasicBlock::create(
                module.get(), "true_" + std::to_string(current_count),
                current_func);
            BasicBlock* exitbb = exitbb = BasicBlock::create(
                module.get(), "exit_" + std::to_string(current_count),
                current_func);
            if (ast_val->get_type()->is_pointer_type()) {
                ast_val = builder->create_load(ast_val);
            }
            if (ast_val->get_type()->is_integer_type()) {
                if (ast_val->get_type() == Type::get_int1_type(module.get())) {
                    ast_val = builder->create_zext(ast_val, int32_type);
                }
                ast_val = builder->create_icmp_ne(ast_val, CONST_INT(0));
            } else
                ast_val = builder->create_fcmp_ne(ast_val, CONST_FP(0.0));
            builder->create_cond_br(ast_val, truebb, exitbb);
            // true
            builder->set_insert_point(truebb);
            is_return = false;
            node.if_statement->accept(*this);
            if (!is_return)
                builder->create_br(exitbb);
            // exit
            builder->set_insert_point(exitbb);
        }
    }
    DEBUG_INFO("end if" + std::to_string(current_count));
    // DEBUG_INFO("visit selection statement over");
}

void CminusfBuilder::visit(ASTIterationStmt& node) {
    // DEBUG_INFO("visit iteration statement");
    int current_count = count;
    count = (count + 1) % MAX_COUNT;
    DEBUG_INFO("while" + std::to_string(current_count));
    if (node.expression != nullptr && node.statement != nullptr) {
        auto cmpbb = BasicBlock::create(
            module.get(), "cmp_" + std::to_string(current_count), current_func);
        auto circlebb = BasicBlock::create(
            module.get(), "circle_" + std::to_string(current_count),
            current_func);
        BasicBlock* exitbb = exitbb = BasicBlock::create(
            module.get(), "exit_" + std::to_string(current_count),
            current_func);
        builder->create_br(cmpbb);
        // cmp
        builder->set_insert_point(cmpbb);
        node.expression->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            ast_val = builder->create_load(ast_val);
        if (ast_val->get_type() == int32_type)
            ast_val = builder->create_icmp_ne(ast_val, {CONST_INT(0)});
        else if (ast_val->get_type()->is_float_type())
            ast_val = builder->create_fcmp_ne(ast_val, {CONST_FP(0.0)});
        builder->create_cond_br(ast_val, circlebb, exitbb);
        // circle
        is_return = false;
        builder->set_insert_point(circlebb);
        node.statement->accept(*this);
        if (!is_return)
            builder->create_br(cmpbb);

        // exitbb
        builder->set_insert_point(exitbb);
    }
    DEBUG_INFO("end while" + std::to_string(current_count));
    // DEBUG_INFO("visit iteration statement over");
}

void CminusfBuilder::visit(ASTReturnStmt& node) {
    DEBUG_INFO("visit return statement");
    if (node.expression == nullptr) {
        if (!return_type->is_void_type())
            ERROR("void function should not return a value");
        return_val = nullptr;
    } else {
        node.expression->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            ast_val = builder->create_load(ast_val);
        if (return_type->is_void_type()) {
            ERROR("non-void function should return a value");
            return_val = nullptr;
        } else {
            type_convert(ast_val, return_type, module, builder);
            builder->create_store(ast_val, return_val);
        }
    }
    is_return = true;
    builder->create_br(return_block);
    DEBUG_INFO("visit return statement over");
}

void CminusfBuilder::visit(ASTVar& node) {
    DEBUG_INFO("visit variable");
    auto var = scope.find(node.id);
    DEBUG_INFO(node.id);
    // if var is 'a[i]', it is necessary to get 'a[i]'
    // as var is 'a' now
    if (node.expression != nullptr) {
        node.expression->accept(*this);
        if (ast_val->get_type()->is_pointer_type())
            ast_val = builder->create_load(ast_val);
        if (ast_val->get_type()->is_float_type())
            ast_val = builder->create_fptosi(ast_val, int32_type);
        auto icmp = builder->create_icmp_lt(ast_val, CONST_INT(0));
        // check the index
        std::string label_name = node.id + std::to_string(count);
        count = (count + 1) % MAX_COUNT;
        auto trueBB = BasicBlock::create(module.get(), label_name + "_true",
                                         current_func);
        auto falseBB = BasicBlock::create(module.get(), label_name + "_false",
                                          current_func);
        auto br = builder->create_cond_br(icmp, trueBB, falseBB);
        // true basic block
        // index is less than 0, exit
        builder->set_insert_point(trueBB);
        builder->create_call(scope.find("neg_idx_except"), {});
        builder->create_br(falseBB);
        // false basic block
        builder->set_insert_point(falseBB);
        // if (builder->create_load(var)->get_type()->is_pointer_type()) {
        if (var->get_type()->get_pointer_element_type()->is_pointer_type()) {
            var = builder->create_load(var);
            var = builder->create_gep(var, {ast_val});
        } else {
            var = builder->create_gep(var, {CONST_INT(0), ast_val});
        }
    }
    // else var is 'a', it is enough
    ast_val = var;
    DEBUG_INFO("visit variable over");
}

void CminusfBuilder::visit(ASTAssignExpression& node) {
    DEBUG_INFO("visit assignment expression");
    if (node.var == nullptr)
        node.expression->accept(*this);
    else {
        node.var->accept(*this);
        auto l_var = ast_val;
        node.expression->accept(*this);
        Value* r_var;
        if (ast_val->get_type()->is_pointer_type())
            r_var = builder->create_load(ast_val);
        else
            r_var = ast_val;
        type_convert(l_var, r_var,
                     l_var->get_type()->get_pointer_element_type(), module,
                     builder);
        builder->create_store(r_var, l_var);
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
        Type* type_;
        if (!l_val->get_type()->is_float_type() &&
            !r_val->get_type()->is_float_type())
            type_ = int32_type;
        else
            type_ = float_type;
        type_convert(l_val, r_val, type_, module, builder);
        switch (node.op) {
            case OP_LE:
                if (type_->is_integer_type())
                    ast_val = builder->create_icmp_le(l_val, r_val);
                else
                    ast_val = builder->create_fcmp_le(l_val, r_val);
                break;
            case OP_LT:
                if (type_->is_integer_type())
                    ast_val = builder->create_icmp_lt(l_val, r_val);
                else
                    ast_val = builder->create_fcmp_lt(l_val, r_val);
                break;
            case OP_GT:
                if (type_->is_integer_type())
                    ast_val = builder->create_icmp_gt(l_val, r_val);
                else
                    ast_val = builder->create_fcmp_gt(l_val, r_val);
                break;
            case OP_GE:
                if (type_->is_integer_type())
                    ast_val = builder->create_icmp_ge(l_val, r_val);
                else
                    ast_val = builder->create_fcmp_ge(l_val, r_val);
                break;
            case OP_EQ:
                if (type_->is_integer_type())
                    ast_val = builder->create_icmp_eq(l_val, r_val);
                else
                    ast_val = builder->create_fcmp_eq(l_val, r_val);
                break;
            case OP_NEQ:
                if (type_->is_integer_type())
                    ast_val = builder->create_icmp_ne(l_val, r_val);
                else
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
        Type* type_;
        if (!l_val->get_type()->is_float_type() &&
            !r_val->get_type()->is_float_type())
            type_ = int32_type;
        else
            type_ = float_type;
        type_convert(l_val, r_val, type_, module, builder);
        switch (node.op) {
            case OP_PLUS:
                if (type_->is_integer_type())
                    ast_val = builder->create_iadd(l_val, r_val);
                else
                    ast_val = builder->create_fadd(l_val, r_val);
                break;
            case OP_MINUS:
                if (type_->is_integer_type())
                    ast_val = builder->create_isub(l_val, r_val);
                else
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
        Type* type_;
        if (!l_val->get_type()->is_float_type() &&
            !r_val->get_type()->is_float_type())
            type_ = int32_type;
        else
            type_ = float_type;
        type_convert(l_val, r_val, type_, module, builder);
        switch (node.op) {
            case OP_MUL:
                if (type_->is_integer_type())
                    ast_val = builder->create_imul(l_val, r_val);
                else
                    ast_val = builder->create_fmul(l_val, r_val);
                break;
            case OP_DIV:
                if (type_->is_integer_type())
                    ast_val = builder->create_isdiv(l_val, r_val);
                else
                    ast_val = builder->create_fdiv(l_val, r_val);
                break;
        }
    }
    DEBUG_INFO("visit term over");
}

void CminusfBuilder::visit(ASTCall& node) {
    DEBUG_INFO("visit function call");
    Function* func = dynamic_cast<Function*>(scope.find(node.id));
    if (func == nullptr) {
        ERROR("it is not a function");
        return;
    }
    DEBUG_INFO(node.id);
    auto argu_list = func->get_args();
    if (argu_list.size() > node.args.size())
        ERROR("too few arguments in function call");
    else if (argu_list.size() < node.args.size())
        ERROR("too many arguments in function call");
    else {
        std::vector<Value*> parameters;
        auto arg = argu_list.begin();
        for (auto& param : node.args) {
            param->accept(*this);
            if (ast_val->get_type()->is_pointer_type())
                if (ast_val->get_type()
                        ->get_pointer_element_type()
                        ->is_array_type()) {
                    ast_val = builder->create_gep(ast_val,
                                                  {CONST_INT(0), CONST_INT(0)});
                } else
                    ast_val = builder->create_load(ast_val);
            type_convert(ast_val, (*arg)->get_type(), module, builder);
            parameters.push_back(ast_val);
            arg++;
        }
        ast_val = builder->create_call(func, parameters);
    }
    DEBUG_INFO("visit function call over");
}
