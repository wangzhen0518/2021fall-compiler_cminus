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
4. `visit(ASTSelectionStmt&)` 和 `visit(ASTIterationStmt&)` 部分，何时创建 `exit block`，何时创建跳转到 `exit` 的命令。其实是由于分支和循环内部，可能存在 `return` 语句，导致跳转到 `return block`，而不是 `exit block`。尽管有时候可能是同一个块，但这次的处理方式是完全区分开。比方说 `if...else...`中，如果 `if` 和 `else` 的块中均有 return block，那么将不会产生 `exit block`。
5. `visit(ASTReturnStmt&)` 部分，何时何处创建 `return` 语句。
6. `visit(ASTVar&)` 部分，如何判断数组下标不为负数。
7. `visit(ASTCall)` 部分，如何将参数处理好，传递给函数。

## 实验设计

### 1. 类型转换

先说明类型转换的处理。

最开始，主要是处理了 expression 中的类型转换问题，即运算符两边操作数类型不同，进行类型转换，只考虑了 float 和 i32 之间的转换，通过调用`fptosi`和`sitofp`实现，并在每个 visit expression 中都写一遍类型转换过程。

之后处理了函数调用时候的类型转换问题，与 expression 中无异。

后来发现只考虑 float 和 i32 之间的转换不可取，比如测试样例[assign_cmp](../../tests3-tslv1/../tests/3-ir-gen/testcases/lv1/assign_cmp.cminus)中的`a=1<3;`，就需要将等号右边的 i1 类型转换为 i32 类型，以实现赋值。
这样遇到了一个问题，之前写的类型转换的地方**全部**要重写，决定改为写一个`type_convert`函数，来专门完成类型转换的工作。这样如果后续又有哪里需要进行新的类型转换，修改这个函数就可以了。

不过这又导致一个新的问题，`type_convert`并非`IRBuilder`类成员，无法直接使用`builder`构造 IR 语句。采取的解决方案是，将`builder`作为参数传入，同样`module`也作为参数传入。如此，函数原型如下`void type_convert(Value*& l，Value*& r， Type* target， std::unique_ptr<Module>& module， IRBuilder*& builder)`，其中`l`，`r`分别为左、右操作数，`target`为需要转换成的类型。这个目前被用在了`visit(ASTSimpleExpression&)`，`visit(ASTTerm&)`和`visit(ASTAdditiveExpression&)`中，这 3 个地方，需要向什么类型转换并不明确，需要先进行判断，将要转换成的类型传入`target`，对左、右操作数进行转换。而对于`visit(ASTAssginExpression&)`，`visit(ASTReturnStmt&)`和`visit(ASTCall&)`，需要转换的类型和需要被转换的操作数很明确，所以采用了另一个重载`void type_convert(Value*& v， Type* target， std::unique_ptr<Module>& module， IRBuilder*& builder)`，特别是如`visit(ASTAssginExpression&)`中左操作数是`point`型，就不符合第一种`type_convert`，虽然目前`type_convert`并没有对`point`进行检查并转换，不会产生问题。

另外还存在一个问题，`i1`和`i32`类型如何区分开。由于`Type`类中的接口`is_[xx]_type`中只有`is_integer_type`这个函数对`integer`进行判断，而`i1`和`i32`，通过这个函数是无法区分开的。目前采取了一种不太好的解决办法，不通过这个接口，而使用`==`进行类型判断，即`xxx_type==int32_type`和`xxx_type==int1_type`进行区分。` int32_type``int1_type `是宏定义，即`Type::get_int32_type(module.get())`，`Type::get_int1_type(module.get())`。

### 2. 作用域

其次是作用域的问题。

比较明确的是`compound-stmt`，每进入一个大括号的时候，开始一个新的作用域，离开大括号时，退出此作用域。

其次是`fun-declaration`。函数定义的末尾进行`scope.exit()`，这很明确，主要是`scope.enter()`应该放在哪。

-   首先明确一下`fun-declaration`做了哪些事。先读取参数列表，存到数组里。再确定返回值类型。之后根据这二者定义函数(指`Function::create`)。创建新的`BasicBlock`。对每个参数分配空间，并存储。访问`compound-stmt`，最后的`return-stmt`后续再说。

-   所以可以看出，应该将`scope.enter()`放在创建完新的`BasicBlock`，并`set_insert_point(...)`之后。

-   但是这会导致一个小问题，由于后续`compound-stmt`又会开启新的作用域，所以对于函数传入参数创建的变量，其作用域和`compound-stmt`中变量的作用域并不完全相同，而是高了一级。不过似乎不会导致错误，至少目前没发现。并且没想到很好的解决办法。如果将`compound-stmt`中的进出作用域都移到上一级，如`if`，`while`中，那么`compound-stmt`内部直接产生`compound-stmt`又会出现问题，比如

```c
if(1) {
    int a;
    {
        int b;
    }
}
```

这里 b 的作用域很明确的比 a 低一级，如果采取上述的解决办法，会导致二者在同一级，这个问题比较严重。

最后是`selection-stmt`和`iteration-stmt`中的作用域问题。这个就比较简单了，完全是进出`compound-stmt`时候进出新的作用域。主要是由于`if(expression)`中的`expression`并不允许创建新的变量，否则`visit(ASTIterationStmt&)`中也需要进行作用域的进出，确保`expression`中定义的变量作用域正确。比如`for(...)`就需要考虑这个问题。

### 3. 函数参数列表

`visit(ASTFunDeclaration& node)`中参数列表处理，要弄清几个步骤。

1. 根据`node`中的参数列表，确定参数的类型，构成*参数类型列表(Type)*，并根据参数的类型，分配相应空间。
2. 之后交给`Function::create`得到函数的*参数列表(Argument)*。
3. 根据*Argument list*得到*Value list*，并将参数值存储起来。(这几个的逻辑并没太弄明白，感觉有点冗余了)。

`visit(ASTCall&)`中的处理类似，但有额外的类型转换需要进行。

### 4. `return-stmt`和`exit-block`

关于`return-stmt`和`exit-block`的问题，由于`exit-block`是处理`return-stmt`导致的问题，所以首先说明处理`return-stmt`的方案。

起初`return-stmt`采用的方式很简单，调用了`visit(ASTReturnStmt&)`就会生成返回语句，这会导致一个函数可能产生多个`ret xxx`，但这并不会产生什么影响。主要是这样会产生空的`exit-block`。比如测试样例中的[comlex3.cminus](../../tests/3-ir-gen/testcases/lv3/complex3.cminus)，对于`gcd`函数就会出现这个问题。

```c
int gcd (int u， int v) {
	if (v == 0) return u;
	else return gcd(v， u - u / v * v);
}
```

生成的.ll 文件中`gcd`函数部分大致如下

```llvm
define i32 @gcd(i32 %arg0， i32 %arg1) {
label_gcd:
	...
	br ...，label %label_true， label %label_false
label_true:
	...
	br label %label_exit
label_false:
	...
	br label %label_exit
label_exit:
}
```

注意到`label_exit`后面是空的，这样的语法并不允许。产生这样语句的原因`if...else...`语句固定产生 3 个 label: true， false， exit。而这里`if-else`后函数就结束了，导致`label_exit`后没有新的语句。

对于上述的这种例子，可以通过增加一个全局变量`is_return`，用以表示访问的块中是否有返回语句，并把生成`exit-block`的位置后移，如果`if`和`else`中都有返回语句的话，就不生成`exit-block`，否则在第一个没有返回语句的地方生成`exit-block`用以产生跳转语句的目标地址。

但是这种办法并不通用，像没有`else`的`if`语句，以及`while`，都在条件判断的地方就需要有`exit-block`的位置`label_exit`，而此时尚未访问块中语句，无法确定是否有返回语句，导致同样可能产生空`label_exit`的问题。

最后的解决办法，在`visit(ASTFunDeclaration&)`中，使用全局变量`return_val`和`return_type`为返回值分配空间，以及记录返回值类型。并在生成函数块的时候，即`auto bb = BasicBlock::create(module.get(), node.id, func);`之后，生成返回块，`return_block = BasicBlock::create(module.get(), "return_" + node.id, func);`，`return_block`同样是全局变量，记录当前函数返回块的位置。不在`visit(ASTReturnStmt&)`中生成返回语句，只将返回语句的返回值存到`return_val`中(不是`void`函数的情况)，生成跳转到`return_block`的跳转语句，而在`visit(ASTFunDeclaration&)`的末尾`return_block`中才生成返回语句。这样空`label_exit`才有地方去，即在`visit(ASTFunDeclaration&)`中，`builder->set_insert_point(return_block);`之前，生成跳转到`return_block`的语句。如果此时`builder`在`exit-block`中的话，就会有一个跳转到`return_block`的语句，不会产生空标签。

### 5. 数组下标检查

对于`visit(ASTVar&)`中$var\to ID[expression]$的情况，`expression`的值可能为负数，需要对此进行检查。

起初试图在编译过程中检查，后来发现这是一个相当愚蠢的想法……。对于这种情况，生成`if`进行判断`expression<0`是否成立，如果成立，调用`neg_idx_except`函数退出，否则取出相应变量，继续程序。

### 6. 数组参数

由于对于数组和指针，`gep`的用法不同，所以需要区别对待。而`cminus`中没有直接定义的指针类型，所以只会出现在将数组作为参数传给函数时，才会出现这个问题。

其实处理比较简单，还是在`visit(ASTVar&)`中有数组下标的情况。数组下标不为负数，需要用`gep`获取指针位置时，进行一次判断，`var`指向的是`point_type`还是`array_type`，采取不同的`gep`用法就可以。

会有这个问题，是一开始把 `point` 和 `array` 全都当做指针进行处理了……后来才发现 IR 中把这 2 个类型区分开了。

### 7. label 编号

这是一个有趣的问题，主要是考虑到一个函数中如果有多个`if`、`while`的情况。这时就不能简单的将`label`命名为`label_true`，`label_false`，`label_exit`之类的，这样跳转会出大问题。

解决办法也很简单，采用一个全局变量`count`进行计数，每遇到`if`、`while`语句就加一，并定了一个上界`#define MAX_COUNT 10'000'000`，超过之后回到 0 进行循环。正常情况下，一个函数中不会超过`MAX_COUNT`，而出了一个函数之后回到 0 循环也不会导致问题。(一般一个程序也不会超过这个值吧……)

### 8. 全局变量

定义的全局变量有

```c
int count;
int ast_num_i;
Function* current_func;
bool is_return = false;
AllocaInst* return_val;
Type* return_type;
BasicBlock* return_block;
Value* ast_val;  // value of function call, expression, et
```

其中`count`已经说明过，`is_return`，`return_val`，`return_type`，`return_block`有所提及，再次说明一下。`is_return`用于标记一个块中是否有访问到返回语句，`return_val`用于存储返回值，以在`return_block`进行返回，`return_type`是返回值的类型，`return_block`是实际进行返回的块。

`current_func`是当前正在定义的函数，用以在后续创建新块时，确定块所在的函数。`ast_num_i`只用于数组定义，记录数组的大小，在`visit(ASTNum&)`时存下来，`visit(ASTVarDeclaration&)`时被使用。

`ast_val`是最常用的一个全局变量，用于在函数之间传递计算出来的值，或者变量之类的。比如`visit(ASTVar&)`数组下标`expression`的值，就通过`ast_val`进行传递，从`expression->accept(*this)`调用的函数，传递回`visit(ASTVar&)`。

### 9. IR 冗余

主要是跳转到`return_block`的语句和`exit-block`的生成的冗余降低，通过`is_return`进行实现。

`exit-block`冗余降低是在`if-else`中，同样采取了`exit-block`定义后移的方式，在第一个没有返回语句的地方生成才`exit-block`，而非进行条件判断时就生成`exit-block`，这样当`if-else`中都有返回语句的话，就不会生成`exit-block`。

在`visit(ASTFunDeclaration&)`中，`builder->set_insert_point(return_block);`之前，生成跳转到`return_block`的语句的时候，先进行判断`if(!is_return)`，即函数最后是否有显式的`return xxx;`语句，如果有的话，在`visit(ASTReturnStmt&)`中已经生成了跳转到`return_block`的语句，此时不再生成，否则生成。

这种方案并不完善，比如下面这种情况，[complex4.cminus](../../tests/3-ir-gen/testcases/lv3/complex4.cminus)中

```c
float abs(float x) {
	if (x > 0)
		return x;
	else
      return 0 - x;
}
```

会生成

```llvm
define float @abs(float %arg0) {
label_abs:
  ...
  br i1 %op7, label %label_true_1, label %label_false_1
label_return_abs:
  ...
  ret float %op8
label_true_1:
  ...
  br label %label_return_abs
label_false_1:
  ...
  br label %label_return_abs
  br label %label_return_abs
}
```

可以看到，最后生成了重复的`br label %label_return_abs`。这是因为`if-else`中的`else`部分生成了一次，之后`visit(ASTSelectionStmt&)`结束时，将`is_return`恢复为`false`(不恢复的话，按照这种方案，会导致很多应该生成`br label %label_return_xxx`的地方没有生成)，之后回到`visit(ASTFunDeclaration&)`时，`if(!is_return)`判断成功，又生成了一次。

目前没想到很好的解决办法。

## 实验总结

这次实验是编译原理的第一次组队实验，我们对不同的函数进行了分工协作，不过由于很多函数的关联度很高，协作的效果可能并不如想象中理想。由于中间代码生成需要注重很多语法和词法上的细节，我们在 debug 上花了很多的时间，对于函数调用及返回，作用域的进入和退出，类型的转换等重难点上尝试了多种方法和设计，最终成功通过了所有的测试用例。总的来说收获颇多，对编程语言的本质的理解也更进了一步。

## 实验反馈 （可选 不会评分）

1. 期待助教给出参考实现？感觉有些解决方案不太好。
2. `i1`和`i32`区分的问题，接口是否可以改进？以通过接口将二者区分开。

## 组间交流 （可选）

无
