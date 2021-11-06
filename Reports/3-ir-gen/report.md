# Lab3 实验报告

| 小组成员 |  姓名  |    学号    |
| :------: | :----: | :--------: |
|    1     |  汪震  | PB19000078 |
|    2     |  蒋皓  | PB19000060 |
|    3     | 方俊浩 | PB19000052 |

## 实验难点

1. 各种地方的类型转换，特别是 `i1` 和 `i32` 的转换。
2. 什么时候进入新的作用域，什么时候退出作用域。
3. `visit(ASTFunDeclaration&)` 部分，如何处理好参数列表，构造出正确的函数类型。
4. `visit(ASTSelectionStmt&)` 和 `visit(ASTIterationStmt&)` 部分，何时创建 `exit block`，何时创建跳转到 `exit` 的命令。其实是由于分支和循环内部，可能存在 `return` 语句，导致跳转到 `return block`，而不是 `exit block`，尽管有时候可能是同一个块，但这次的处理方式是完全区分开。比方说 `if...else...`中，如果 `if` 和 `else` 的块中均有 return block，那么将不会产生 ``exit block`。
5. `visit(ASTReturnStmt&)` 部分，何时何处创建 `return` 语句。
6. `visit(ASTVar&)` 部分，如何判断数组下标不为负数。
7. `visit(ASTCall)` 部分，如何将参数处理好，传递给函数。

## 实验设计

1. 先说明类型转换的处理。
    - 最开始，主要是处理了 expression 中的类型转换问题，即运算符两边操作数类型不同，进行类型转换，只考虑了 float 和 i32 之间的转换，通过调用`fptosi`和`sitofp`实现，并在每个 visit expression 中都写一遍类型转换过程。
    - 之后处理了函数调用时候的类型转换问题，与 expression 中无异。
    - 之后发现只考虑 float 和 i32 之间的转换不可取，比如测试样例[assign_cmp](../../tests3-tslv1/../tests/3-ir-gen/testcases/lv1/assign_cmp.cminus)中的`a=1<3;`，就需要将等号右边的 i1 类型转换为 i32 类型，以实现赋值。
    - 这样遇到了一个问题，之前写的类型转换的地方**全部**要重写，决定改为写一个`type_convert`函数，来专门完成类型转换的工作。这样如果后续又有哪里需要进行新的类型转换，修改这个函数就可以了。
    - 不过这又导致一个新的问题，`type_convert`并非`IRBuilder`类成员，无法直接使用`builder`构造 IR 语句。采取的解决方案是，将`builder`作为参数传入，同样`module`也作为参数传入。如此，函数原型如下`void type_convert(Value*& l,Value*& r, Type* target, std::unique_ptr<Module>& module, IRBuilder*& builder)`，其中`l`，`r`分别为左、右操作数，`target`为需要转换成的类型。这个目前被用在了`visit(ASTSimpleExpression&)`, `visit(ASTTerm&)`和`visit(ASTAdditiveExpression&)`中，这 3 个地方，需要向什么类型转换并不明确，需要先进行判断，将要转换成的类型传入`target`，对左、右操作数进行转换。而对于`visit(ASTAssginExpression&)`, `visit(ASTReturnStmt&)`和`visit(ASTCall&)`，需要转换的类型和需要被转换的操作数很明确，所以采用了另一个重载`void type_convert(Value*& v, Type* target, std::unique_ptr<Module>& module, IRBuilder*& builder)`, 特别是如`visit(ASTAssginExpression&)`中左操作数是`point`型，就不符合第一种`type_convert`，虽然目前`type_convert`并没有对`point`进行检查并转换，不会产生问题。
    - 另外还存在一个问题，`i1`和`i32`类型如何区分开。由于`Type`类中的接口`is_[xx]_type`中只有`is_integer_type`这个函数对`integer`进行判断，而`i1`和`i32`，通过这个函数是无法区分开的。目前采取了一种不太好的解决办法，不通过这个接口，而使用`==`进行类型判断，即`xxx_type==int32_type`和`xxx_type==int1_type`进行区分。` int32_type``int1_type `是宏定义，即`Type::get_int32_type(module.get())`, `Type::get_int1_type(module.get())`
2. 其次是作用域的问题。
    - 比较明确的是`compound-stmt`，每进入一个大括号的时候，开始一个新的作用域，离开大括号时，退出此作用域。
    - 其次是`fun-declaration`。函数定义的末尾进行`scope.exit()`，这很明确，主要是`scope.enter()`应该放在哪。
        - 首先明确一下`fun-declaration`做了哪些事。先读取参数列表，存到数组里。再确定返回值类型。之后根据这二者定义函数(指`Function::create`)。创建新的`BasicBlock`。对每个参数分配空间，并存储。访问`compound-stmt`，最后的`return-stmt`后续再说。
        - 所以可以看出，应该将`scope.enter()`放在创建完新的`BasicBlock`，并`set_insert_point(...)`之后。
        - 但是这会导致一个小问题，由于后续`compound-stmt`又会开启新的作用域，所以对于函数传入参数创建的变量，其作用域和`compound-stmt`中变量的作用域并不完全相同，而是高了一级。不过似乎不会导致错误，至少目前没发现。并且没想到很好的解决办法。如果将`compound-stmt`中的进出作用域都移到上一级，如`if`, `while`中，那么`compound-stmt`内部直接产生`compound-stmt`又会出现问题，比如
        ```c
        if(1) {
            int a;
            {
                int b;
            }
        }
        ```
        这里 b 的作用域很明确的比 a 低一级，如果采取上述的解决办法，会导致二者在同一级，这个问题比较严重。
    - 之后是`selection-stmt`和`iteration-stmt`中的作用域问题。这个就比较简单了，完全是进出`compound-stmt`时候进出新的作用域。主要是由于`if(expression)`中的`expression`并不允许创建新的变量，否则`visit(ASTIterationStmt&)`中也需要进行作用域的进出，确保`expression`中定义的变量作用域正确。比如`for(...)`就需要考虑这个问题。
3. `visit(ASTFunDeclaration& node)`中参数列表处理，要弄清几个步骤。
    1. 根据`node`中的参数列表，确定参数的类型，构成*参数类型列表(Type)*，并根据参数的类型，分配相应空间
    2. 之后交给`Function::create`得到函数的*参数列表(Argument)*
    3. 根据*Argument list*得到*Value list*，并将参数值存储起来。(这几个的逻辑并没太弄明白，感觉有点冗余了)。
4. 关于`return-stmt`和`exit-block`的问题，由于二者的问题相互关联
    - 最开始的方式，是简单地在调用`visit(ASTReturnStmt&)`时，创建返回语句。这样会导致一个函数中可能有多个`ret xxx`，这并不是什么主要问题。

请写明为了顺利完成本次实验，加入了哪些亮点设计，并对这些设计进行解释。
可能的阐述方向有:

5. 如何设计全局变量
6. 遇到的难点以及解决方案
7. 如何降低生成 IR 中的冗余
8. ...

### 实验总结

此次实验有什么收获

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

无
