# Lab4 实验报告-阶段一

| 小组成员 |  姓名  |    学号    |
| :------: | :----: | :--------: |
|    1     |  汪震  | PB19000078 |
|    2     |  蒋皓  | PB19000060 |
|    3     | 方俊浩 | PB19000052 |

## 实验要求

1. 阅读`Mem2Reg`与`LoopSearch`两个优化，以及和`Mem2Reg`相关的`Dominators`(生成结点之间的支配信息)。
2. `LoopSearch`得到循环的信息，之后通过`LoopInvHoist`实现循环不变式外提。
3. `Mem2Reg`将先前生成的 IR 转换为 SSA 形式，之后通过`ConstPropagation`实现常量传播和死代码删除。
4. 通过`ActiveVars`实现活跃变量分析。

## 思考题

### LoopSearch

1. `LoopSearch`中直接用于描述一个循环的数据结构是什么？需要给出其具体类型。

`std::unordered_set<BBset_t *> loop_set;`
`loop_set`中每个成员是一个`loop`，对应的`BBset_t`存储了循环中的所有`Basic_Block`。

2. 循环入口是重要的信息，请指出`LoopSearch`中如何获取一个循环的入口？需要指出具体代码，并解释思路。

对应函数为`find_loop_base(...)`，如果存在一个`Basic_Block`的前驱不在循环里面，那么这个点就是对应的循环入口。另外存在一种情况，循环入口的前驱在之前的过程中被删除了，那么在被删除的点`reserved`中，寻找存在后继在循环中的点，如果找到，那么这个后继就是循环入口。

```cpp
CFGNodePtr LoopSearch::find_loop_base(CFGNodePtrSet* set, CFGNodePtrSet& reserved) {
    CFGNodePtr base = nullptr;
    for (auto n : *set)
        for (auto prev : n->prevs)
            if (set->find(prev) == set->end())
                base = n;

    if (base != nullptr)
        return base;

    for (auto res : reserved)  // why search succs?
        for (auto succ : res->succs)
            if (set->find(succ) != set->end())
                base = succ;

    return base;
}
```

3. 仅仅找出强连通分量并不能表达嵌套循环的结构。为了处理嵌套循环，`LoopSearch`在 Tarjan algorithm 的基础之上做了什么特殊处理？

关键在于`LoopSearch::run()`中的`nodes.erase(base);`，删除了当前找到的强连通分量的入口，从而破坏了外层的循环结构，再次调用`strongly_connected_components`就可以得到内层的循环。
另外删去结点被存储在`reserved`中，用于`LoopSearch::find_loop_base`寻找循环入口。

4. 某个基本块可以属于多层循环中，`LoopSearch`找出其所属的最内层循环的思路是什么？这里需要用到什么数据？这些数据在何时被维护？需要指出数据的引用与维护的代码，并简要分析。

用到的数据是`bb2base`，每次找到一个循环之后，都会将`bb2base[bb]`映射到循环入口对应的基本块，即`base->bb`。
由于最先找到的是外层循环，所以如果一个基本块属于内层循环，后续找到那个循环时，会再次赋值，从而映射到所属的最内层循环。

```cpp
// step 5: map each node to loop base
for (auto bb : *bb_set)
    bb2base[bb] = base->bb;
```

### Mem2reg

1. 请**简述**概念：支配性、严格支配性、直接支配性、支配边界。

    1. **支配性**: 如果流图中每条从根节点到结点`n`的路径，都会经过结点`m`，那么`m`就是`n`的支配结点。
    2. **严格支配性**: 根据上述定义，显然每个结点是自身的支配结点。严格支配性就是指除自身以外的其余支配结点。
    3. **直接支配性**: 距离结点`n`最近的严格支配结点，就是`n`的直接支配结点。
    4. **支配边界**: 相对于结点`n`具有以下性质的点`m`的集合为`n`的支配边界
        1. `n`支配`m`的一个前驱
        2. `n`不严格支配`m`

2. `phi`节点是 SSA 的关键特征，请**简述**`phi`节点的概念，以及引入`phi`节点的理由。

    1. **概念**：对于结点`n`，如果存在多个前驱中均对变量`x`进行赋值，那么块`n`开始处就需要引入`phi`结点，判断应取多个前驱中哪个`x`的值。
    2. **理由**：由于`SSA`对每个变量只赋值一次，所以虽然是同一个变量`x`，但在不同的赋值语句中，被赋值的变量`x`名字是不同的，所以需要引入`phi`函数，决定汇合的地方，应该取哪个`x`的值。

3. 下面给出的`cminus`代码显然不是 SSA 的，后面是使用 lab3 的功能将其生成的 LLVM IR（**未加任何 Pass**），说明对一个变量的多次赋值变成了什么形式？

多次赋值通过多次执行`store`指令实现，`store`均存入同一个变量。
这不完全是`SSA`形式，不过像加法等运算，确实是变量只赋值了一次。
查到的资料很好地说明了这一点，[LLVM SSA 介绍](https://blog.csdn.net/qq_29674357/article/details/78731713)。

> 这里采取的是 alloca/load/store 的方法，把所有局部变量通过“alloca”指令进行分配。 ...... 不难发现，构造 SSA 的算法不能算快，而且需要各种复杂的数据结构，这些因素导致前端在生成 SSA 形式的 IR 时十分笨拙。因此，LLVM 采用了一个“小技巧（trick）”，可以把构造 SSA 的工作从前端分离出来。
>
> LLVM does require all register values to be in SSA form, it does not require (or permit) memory objects to be in SSA form.
>
> 即：LLVM 里，虚拟寄存器是 SSA 形式，而内存则并非 SSA 形式。

4. 对下面给出的`cminus`程序，使用 lab3 的功能，分别关闭/开启`Mem2Reg`生成 LLVM IR。对比生成的两段 LLVM IR，开启`Mem2Reg`后，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

    1. 局部非数组变量的`load`, `store`均删除了，由于转换成了 SSA 形式，所以局部变量的赋值和使用均通过不同的变量名代替了，不再需要进行`load`和`store`。如`func`中的变量`x`，没开`Mem2Reg`时，先用`op1`将传入参数的值存储下来，之后赋值时，对`op1`进行`store`，需要使用`op1`的值时，再`load`到一个变量里(`op2`, `op8`)，使用变量的值。而开启`Mem2Reg`后，`if`中进行值判断时，直接使用的是传入的参数`op0`，而需要对`x`进行赋值时，这里是通过`phi`函数对`op9`进行赋值，再返回`op9`的值。
    2. 全局变量和局部的数组变量的`load`和`store`指令没有变化(数组的`load`是`getelementptr`)。全局变量由于生存周期是整个文件，所以如果通过对不同名变量的赋值代替对全局变量的`store`，那么在离开这个函数之后，全局变量的值并没有改变，但是用来代替的变量的生存周期已经结束(即使是`static`型的变量，也离开了作用域，结果一样)，所以在另一个函数里使用全局变量值的时候，将不会是正确的值，所以全局变量的`load`, `store`指令无法删除。
    3. 局部数组变量大概是还没做这个优化。

5. 指出放置 phi 节点的代码，并解释是如何使用支配树的信息的。需要给出代码中的成员变量或成员函数名称。

对应函数为`Mem2Reg::generate_phi()`，变量为`bb_has_var_phi`, `global_live_var_name`, `work_list`, `live_var_2blocks`, `bb_dominance_frontier_bb`, `dominators_`(成员变量), `phi`。

实际生成`phi`结点的代码是

```c++
 auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(),
                                bb_dominance_frontier_bb);
phi->set_lval(var);
bb_dominance_frontier_bb->add_instr_begin(phi);
```

对应的循环是

```c++
// bb has phi for var
std::map<std::pair<BasicBlock *, Value *>, bool>bb_has_var_phi;
for (auto var : global_live_var_name) {
    std::vector<BasicBlock *> work_list;
    work_list.assign(live_var_2blocks[var].begin(),
                     live_var_2blocks[var].end());
    // just
    // std::vector<BasicBlock *> work_list(live_var_2blocks[var].begin(), \
    // live_var_2blocks[var].end()); is ok
    for (int i = 0; i < work_list.size(); i++) {
        auto bb = work_list[i];
        for (auto bb_dominance_frontier_bb :
             dominators_->get_dominance_frontier(bb)) {
            if (bb_has_var_phi.find({bb_dominance_frontier_bb, var}) ==
                bb_has_var_phi.end()) {
                // generate phi for bb_dominance_frontier_bb & add
                // bb_dominance_frontier_bb to work list
                auto phi = PhiInst::create_phi(
                    var->get_type()->get_pointer_element_type(),
                    bb_dominance_frontier_bb);
                phi->set_lval(var);
                bb_dominance_frontier_bb->add_instr_begin(phi);
                work_list.push_back(bb_dominance_frontier_bb);
                bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
            }
        }
    }
}
```

通过支配树得到`dominance_frontier`的信息，每个结点，对其使用了变量`var`且尚未插入`phi`语句的`DF`结点，插入`phi`语句。

### 代码阅读总结

1. SSA 的构建，课程内容并没有涉及，通过实验有所了解。
2. 更加熟悉了图的各种操作。
3. `LoopSearch`中找`bb2base`，了解到一般算法(此处为`Tarjan Algorithm`)与实际使用中，可以通过稍加修改，以满足使用需求。

### 实验反馈 （可选 不会评分）

1. 代码能否考虑增加适量注释……说明一下思路？

### 组间交流 （可选）

无
