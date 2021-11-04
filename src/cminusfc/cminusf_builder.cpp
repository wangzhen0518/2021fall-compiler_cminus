#include "cminusf_builder.hpp"

<<<<<<< HEAD
#define ERROR(comment) std::cout << comment
=======
// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_INT(num) \
    ConstantInt::get(num, module.get())
>>>>>>> master

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get((int)num, module.get())
#define CONST_ZERO(type) ConstantZero::get(type, module.get())

#define int32_type Type::get_int32_type(module.get())
#define float_type Type::get_float_type(module.get())
#define void_type Type::get_void_type(module.get())
#define int32_ptr_type Type::get_int32_ptr_type(module.get())
#define float_ptr_type Type::get_float_ptr_type(module.get())

int ast_num_i;
float ast_num_f;
CminusType ast_num_type;
Constant* ast_num_val;
Function* main_fun;
AllocaInst* return_val;
Value* val;  // value of function call, expression, etc
Value* var;  // the variable to be assigned

void CminusfBuilder::visit(ASTProgram& node) {
    for (auto& decl : node.declarations)
        decl->accept(*this);
}

void CminusfBuilder::visit(ASTNum& node) {
    if (node.type == TYPE_INT) {
        ast_num_type = TYPE_INT;
        ast_num_i = node.i_val;
        ast_num_val = CONST_INT(node.i_val);
    } else if (node.type == TYPE_FLOAT) {
        ast_num_type = TYPE_FLOAT;
        ast_num_f = node.f_val;
        ast_num_val = CONST_FP(node.f_val);
    }
}

void CminusfBuilder::visit(ASTVarDeclaration& node) {
    if (scope.in_global()) {
        if (node.num == nullptr) {
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
        } else {
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
}

void CminusfBuilder::visit(ASTFunDeclaration& node) {
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
    else if (node.type == TYPE_INT) {
        return_type = int32_type;
        return_val = builder->create_alloca(int32_type);
    } else {  // TYPE_FLOAT
        return_type = float_type;
        return_val = builder->create_alloca(float_type);
    }

    auto Fun = Function::create(FunctionType::get(return_type, param_types),
                                node.id, module.get());
    if (node.id == "main")
        main_fun = Fun;

    auto bb = BasicBlock::create(module.get(), node.id, Fun);
    builder->set_insert_point(bb);
    scope.push(node.id, Fun);
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
    for (auto arg = Fun->arg_begin(); arg != Fun->arg_end(); arg++)
        args.push_back(*arg);
    for (int i = 0; i < node.params.size(); i++)
        builder->create_store(args[i], scope.find(node.params[i]->id));
    node.compound_stmt->accept(*this);
    scope.exit();
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

void CminusfBuilder::visit(ASTCompoundStmt& node) {}

void CminusfBuilder::visit(ASTExpressionStmt& node) {}

void CminusfBuilder::visit(ASTSelectionStmt& node) {}

void CminusfBuilder::visit(ASTIterationStmt& node) {}

void CminusfBuilder::visit(ASTReturnStmt& node) {}

void CminusfBuilder::visit(ASTVar& node) {
    var = scope.find(node.id);
    // if var is 'a[i]', it is necessary to get 'a[i]'
    // as var is 'a' now
    if (node.expression != nullptr) {
        node.expression->accept(*this);
        auto gep = builder->create_gep(var, {CONST_INT(0), val});
    }
    // else var is 'a', it is enough
}

void CminusfBuilder::visit(ASTAssignExpression& node) {
    if (node.var != nullptr)
        node.var->accept(*this);
    node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSimpleExpression& node) {
    if (node.additive_expression_r == nullptr)
        node.additive_expression_l->accept(*this);
    else {
        node.additive_expression_l->accept(*this);
        auto l_val = val;
        node.additive_expression_r->accept(*this);
        auto r_val = val;
        switch (node.op) {
            case OP_LE:
                val = builder->create_icmp_eq(l_val, r_val);
                break;
            case OP_LT:
                break;
            case OP_GT:
                break;
            case OP_GE:
                break;
            case OP_EQ:
                break;
            case OP_NEQ:
                break;
        }
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression& node) {
    if (node.additive_expression == nullptr)
        node.term->accept(*this);
    else {
        node.additive_expression->accept(*this);
        // TODO:
        node.term->accept(*this);
        // TODO:
        switch (node.op) {
            case OP_PLUS:
                /* code */
                break;
            case OP_MINUS:
                break;
        }
    }
}

void CminusfBuilder::visit(ASTTerm& node) {
    if (node.term == nullptr)
        node.factor->accept(*this);
    else {
        node.term->accept(*this);
        auto l_val = val;
        node.factor->accept(*this);
        auto r_val = val;
        switch (node.op) {
            case OP_MUL:
                if (l_val->get_type() == int32_type &&
                    r_val->get_type() == int32_type)
                    val = builder->create_imul(l_val, r_val);
                else if (l_val->get_type() == int32_type &&
                         r_val->get_type() == float_type) {
                    FloatType* l_val_temp;  // TODO: type convert
                } else                      // both are float_type
                    val = builder->create_fmul(l_val, r_val);
                break;
            case OP_DIV:
                if (l_val->get_type() == int32_type &&
                    r_val->get_type() == int32_type)
                    val = builder->create_isdiv(l_val, r_val);
                else if (l_val->get_type() == int32_type &&
                         r_val->get_type() == float_type) {
                    FloatType* l_val_temp;  // TODO: type convert
                } else                      // both are float_type
                    val = builder->create_fdiv(l_val, r_val);
                break;
        }
    }
}

void CminusfBuilder::visit(ASTCall& node) {
    auto* Func = scope.find(node.id);
    std::vector<Value*> arguments;
    for (auto& arg : node.args) {
        arg->accept(*this);
        arguments.push_back(val);
    }
    // TODO: type conversion
    val = builder->create_call(Func, arguments);
}
