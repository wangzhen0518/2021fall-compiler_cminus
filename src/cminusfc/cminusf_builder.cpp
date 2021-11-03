#include "cminusf_builder.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get((int)num, module.get())
#define CONST_ZERO(type) ConstantZero::get(type, module.get())

int ast_num_i;
float ast_num_f;
CminusType ast_num_type;
Constant *ast_num_val;
Function *main_fun;
AllocaInst *return_val;

void CminusfBuilder::visit(ASTProgram &node) {
    for (int declaration = 0; declaration < node.declarations.size();
         declaration++) {
        node.declarations[declaration]->accept(*this);
    }
}

void CminusfBuilder::visit(ASTNum &node) {
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

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    if (scope.in_global()) {
        if (node.num == nullptr) {
            if (node.type == TYPE_INT) {
                Value *val = GlobalVariable::create(
                    node.id, module.get(), Type::get_int32_type(module.get()),
                    false, CONST_ZERO(Type::get_int32_type(module.get())));
                scope.push(node.id, val);
            } else {
                Value *val = GlobalVariable::create(
                    node.id, module.get(), Type::get_float_type(module.get()),
                    false, CONST_ZERO(Type::get_float_type(module.get())));
                scope.push(node.id, val);
            }
        } else {
            if (node.type == TYPE_INT) {
                node.num->accept(*this);
                Value *val = GlobalVariable::create(
                    node.id, module.get(),
                    ArrayType::get(Type::get_int32_type(module.get()),
                                   ast_num_i),
                    false,
                    CONST_ZERO(ArrayType::get(
                        Type::get_int32_type(module.get()), ast_num_i)));
                scope.push(node.id, val);
            } else {
                node.num->accept(*this);
                Value *val = GlobalVariable::create(
                    node.id, module.get(),
                    ArrayType::get(Type::get_float_type(module.get()),
                                   ast_num_i),
                    false,
                    CONST_ZERO(ArrayType::get(
                        Type::get_float_type(module.get()), ast_num_i)));
                scope.push(node.id, val);
            }
        }
    } else {
        if (node.num == nullptr) {
            if (node.type == TYPE_INT) {
                Value *val =
                    builder->create_alloca(Type::get_int32_type(module.get()));
                scope.push(node.id, val);
            } else {
                Value *val =
                    builder->create_alloca(Type::get_float_type(module.get()));
                scope.push(node.id, val);
            }

        } else {
            if (node.type == TYPE_INT) {
                node.num->accept(*this);
                Value *val = builder->create_alloca(ArrayType::get(
                    Type::get_int32_type(module.get()), ast_num_i));
                scope.push(node.id, val);
            } else {
                node.num->accept(*this);
                Value *val = builder->create_alloca(ArrayType::get(
                    Type::get_float_type(module.get()), ast_num_i));
                scope.push(node.id, val);
            }
        }
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    std::vector<Type *> param_types;
    for (auto param : node.params) {
        if (param->isarray) {
            if (param->type == TYPE_INT) {
                param_types.push_back(Type::get_int32_ptr_type(module.get()));
            } else {
                param_types.push_back(Type::get_float_ptr_type(module.get()));
            }

        } else {
            if (param->type == TYPE_INT) {
                param_types.push_back(Type::get_int32_type(module.get()));
            } else {
                param_types.push_back(Type::get_float_type(module.get()));
            }
        }
    }
    Type *return_type;
    if (node.type == TYPE_VOID) {
        return_type = Type::get_void_type(module.get());
    } else if (node.type == TYPE_INT) {
        return_type = Type::get_int32_type(module.get());
    } else if (node.type == TYPE_FLOAT) {
        return_type = Type::get_float_type(module.get());
    }
    auto Fun = Function::create(FunctionType::get(return_type, param_types),
                                node.id, module.get());
    main_fun = Fun;
    auto bb = BasicBlock::create(module.get(), node.id, Fun);
    builder->set_insert_point(bb);
    if (node.type == TYPE_INT) {
        return_val = builder->create_alloca(Type::get_int32_type(module.get()));
    } else if (node.type == TYPE_FLOAT) {
        return_val = builder->create_alloca(Type::get_float_type(module.get()));
    }
    scope.push(node.id, Fun);
    scope.enter();
    std::vector<Value *> args;
    for (auto arg = Fun->arg_begin(); arg != Fun->arg_end(); arg++) {
        args.push_back(*arg);
    }
    for (int i = 0; i < node.params.size(); i++) {
        builder->create_store(args[i], scope.find(node.params[i]->id));
    }
    node.compound_stmt->accept(*this);
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) {
    Value *param_ptr;
    if (node.isarray) {
        if (node.type == TYPE_INT) {
            param_ptr =
                builder->create_alloca(Type::get_int32_ptr_type(module.get()));
        } else {
            param_ptr =
                builder->create_alloca(Type::get_float_ptr_type(module.get()));
        }

    } else {
        if (node.type == TYPE_INT) {
            param_ptr =
                builder->create_alloca(Type::get_int32_type(module.get()));
        } else {
            param_ptr =
                builder->create_alloca(Type::get_float_type(module.get()));
        }
    }
    scope.push(node.id, param_ptr);
}

void CminusfBuilder::visit(ASTCompoundStmt &node) {}

void CminusfBuilder::visit(ASTExpressionStmt &node) {}

void CminusfBuilder::visit(ASTSelectionStmt &node) {}

void CminusfBuilder::visit(ASTIterationStmt &node) {}

void CminusfBuilder::visit(ASTReturnStmt &node) {}

void CminusfBuilder::visit(ASTVar &node) {}

void CminusfBuilder::visit(ASTAssignExpression &node) {}

void CminusfBuilder::visit(ASTSimpleExpression &node) {}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {}

void CminusfBuilder::visit(ASTTerm &node) {}

void CminusfBuilder::visit(ASTCall &node) {}
